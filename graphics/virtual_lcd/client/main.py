import datetime
import re
import struct
import sys
import time
from dataclasses import dataclass
from typing import Any, Optional, Tuple

import numpy as np
import qdarktheme
from loguru import logger  # noqa: F401
from main_ui import Ui_MainWindow
from PySide6 import QtSerialPort
from PySide6.QtCore import (
    QIODeviceBase,
    QPointF,
    Qt,
    QTimer,
    Signal,
    Slot,
)
from PySide6.QtGui import (
    QImage,
    QKeyEvent,
    QMouseEvent,
    QPixmap,
    QTransform,
    QWheelEvent,
)
from PySide6.QtWidgets import (
    QApplication,
    QInputDialog,
    QMainWindow,
    QMessageBox,
    QStyle,
    QWidget,
)
from serial.tools.list_ports import comports


def set_font_color(btn: Any, color: str):
    btn.setStyleSheet(f"color: {color};")


def get_com(hwid_target) -> Optional[str]:
    for port, desc, hwid in sorted(comports()):
        if hwid_target in hwid:
            return port
    return None


def get_all_ids() -> list[Tuple[str, str, str]]:  # (vid:pid, port, desc)
    ids = []
    for port, desc, hwid in sorted(comports()):
        r = re.search(r"VID:PID=([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})", hwid)
        if r:
            vid = r.group(1)
            pid = r.group(2)
            ids.append((f"{vid}:{pid}", port, desc))
    return ids


class Format:
    RGB565 = 0
    RGB888 = 1
    MONO = 2
    MONO_INV = 3
    GRAY_8BIT = 4


class InDevMask:
    KEYBOARD = 1 << 0
    TOUCH = 1 << 1
    MOUSE = 1 << 2
    BUTTON = 1 << 3
    ENCODER = 1 << 4


@dataclass()
class Screen:
    enable: bool = False
    width: int = 800
    height: int = 480
    rotation: int = 0
    format: int = Format.RGB565
    indev: int = (
        InDevMask.KEYBOARD
        | InDevMask.TOUCH
        | InDevMask.MOUSE
        | InDevMask.BUTTON
        | InDevMask.ENCODER
    )


class SpeedCounter:
    def __init__(self, refreshTime=2) -> None:
        self.t = time.perf_counter()
        self.speed = 0.0
        self.sum = 0.0
        self.refreshTime = refreshTime

    def update(self, value: float) -> None:
        self.sum += value
        if time.perf_counter() - self.t > self.refreshTime:
            self.speed = self.sum / self.refreshTime
            self.t = time.perf_counter()
            self.sum = 0.0
            self.count = 0

    def getBps(self) -> float:
        return self.speed

    def getKbps(self) -> float:
        return self.speed / 1024 * 8

    def getKBps(self) -> float:
        return self.speed / 1024

    def getMbps(self) -> float:
        return self.speed / 1024 / 1024 * 8

    def getMBps(self) -> float:
        return self.speed / 1024 / 1024


class ScreenManager(QWidget):
    param_update_signal = Signal()
    frame_update_signal = Signal()
    SCREEN_NUM = 10

    def __init__(self, serial: QtSerialPort.QSerialPort) -> None:
        super().__init__()
        self.scr = Screen()
        self.touch_state = [False, 0, 0]
        self.mouse_state = [0, 0, 0, 0]  # Key, X, Y, Wheel
        self.encoder_state = [False, 0]
        self.serial = serial
        self._last_pos_time = time.perf_counter()
        self.buffer = bytearray()
        self.state = 0
        self.packlen = 0
        self.init_params()

    def init_params(self):
        self.bitwidth = {
            Format.RGB565: 2,
            Format.RGB888: 3,
            Format.MONO: 1 / 8,
            Format.MONO_INV: 1 / 8,
            Format.GRAY_8BIT: 1,
        }[self.scr.format]
        self.framebuffer = np.zeros(
            self.scr.width * self.scr.height * self.bitwidth,
            dtype=np.uint8,
        )
        self.buf_length = self.scr.width * self.scr.height * self.bitwidth
        self.set_window(0, 0, self.scr.width, self.scr.height)
        self.param_update_signal.emit()

    def set_window(self, x: int, y: int, w: int, h: int):
        if not self.scr.enable:
            return
        self.win = [x, y, w, h]
        x0, x1 = self.win[0], self.win[2] + self.win[0] - 1
        y0, y1 = self.win[1], self.win[3] + self.win[1] - 1
        assert x0 <= x1 and y0 <= y1
        assert x0 >= 0 and x1 <= self.scr.width
        assert y0 >= 0 and y1 <= self.scr.height
        self.window_front_size = (
            self.scr.width * y0 * self.bitwidth + x0 * self.bitwidth
        )
        self.window_back_size = (
            self.scr.width * (self.scr.height - y1 - 1) * self.bitwidth
            + (self.scr.width - x1 - 1) * self.bitwidth
        )
        self.cursor_pos = self.window_front_size
        self.x0, self.x1, self.y0, self.y1 = x0, x1, y0, y1
        self.wrap_size = (x0 + self.scr.width - x1 - 1) * self.bitwidth
        # logger.debug(
        #     f"buf_length: {self.buf_length} window_front_size: {self.window_front_size} window_back_size: {self.window_back_size}, cursor_pos: {self.cursor_pos}, wrap_size: {self.wrap_size}, x0: {self.x0}, x1: {self.x1}, y0: {self.y0}, y1: {self.y1}"
        # )

    def write_framebuffer(self, data: bytes):
        if not self.scr.enable:
            return
        pdata = np.frombuffer(data, dtype=np.uint8)
        if self.wrap_size == 0:  # 直接写入
            while pdata.size > 0:
                max_write_length = (
                    self.buf_length - self.window_back_size - self.cursor_pos
                )
                wr_data = pdata[:max_write_length]
                pdata = pdata[wr_data.size :]
                self.framebuffer[self.cursor_pos : self.cursor_pos + wr_data.size] = (
                    wr_data
                )
                self.cursor_pos += wr_data.size
                if self.cursor_pos >= self.buf_length - self.window_back_size:
                    self.cursor_pos %= self.buf_length - self.window_back_size
                    self.cursor_pos += self.window_front_size
        else:  # 按行写入
            while pdata.size > 0:
                max_write_length = (self.x1 + 1) * self.bitwidth - self.cursor_pos % (
                    self.width * self.bitwidth
                )  # 指针到窗口右边界的距离
                wr_data = pdata[:max_write_length]
                pdata = pdata[wr_data.size :]
                self.framebuffer[self.cursor_pos : self.cursor_pos + wr_data.size] = (
                    wr_data
                )
                self.cursor_pos += wr_data.size
                if wr_data.size == max_write_length:
                    self.cursor_pos += self.wrap_size
                if self.cursor_pos >= self.buf_length - self.window_back_size:
                    self.cursor_pos %= self.buf_length - self.window_back_size
                    self.cursor_pos += self.window_front_size
        self.flush()

    def to_image(self) -> QImage:
        if self.scr.format == Format.RGB565:
            img = QImage(
                self.framebuffer.tobytes(),
                self.scr.width,
                self.scr.height,
                QImage.Format.Format_RGB16,
            )
        elif self.scr.format == Format.RGB888:
            img = QImage(
                self.framebuffer.tobytes(),
                self.scr.width,
                self.scr.height,
                QImage.Format.Format_RGB888,
            )
        elif self.scr.format == Format.MONO:
            img = QImage(
                self.framebuffer.tobytes(),
                self.scr.width,
                self.scr.height,
                QImage.Format.Format_Mono,
            )
        elif self.scr.format == Format.MONO_INV:
            img = QImage(
                self.framebuffer.tobytes(),
                self.scr.width,
                self.scr.height,
                QImage.Format.Format_MonoLSB,
            )
        elif self.scr.format == Format.GRAY_8BIT:
            img = QImage(
                self.framebuffer.tobytes(),
                self.scr.width,
                self.scr.height,
                QImage.Format.Format_Grayscale8,
            )
        else:
            raise ValueError(f"Unknown format: {self.scr.format}")
        if self.scr.rotation == 0:
            return img
        return img.transformed(QTransform().rotate(self.scr.rotation))

    def unpack(self, type: int, data: bytes):
        if type == 1:  # init
            """
            uint16_t width;
            uint16_t height;
            uint8_t format;
            uint8_t rotate;
            uint8_t indev_flags;
            uint8_t flush_event;
            """
            (
                self.scr.width,
                self.scr.height,
                self.scr.format,
                rot,
                self.scr.indev,
            ) = struct.unpack("<HHBBB", data[:7])
            self.scr.rotation = {
                0: 0,
                1: 90,
                2: 180,
                3: 270,
            }[rot]
            self.scr.enable = True
            self.init_params()
        elif type == 2:  # set window
            """
            uint16_t x;
            uint16_t y;
            uint16_t width;
            uint16_t height;
            """
            self.set_window(*struct.unpack("<HHHH", data[:8]))
        elif type == 3:  # write
            self.write_framebuffer(data)
        elif type == 4:  # set window + write
            self.set_window(*struct.unpack("<HHHH", data[:8]))
            self.write_framebuffer(data[8:])
        elif type == 5:  # draw point
            """
            uint16_t x;
            uint16_t y;
            uint32_t color;
            """
            x, y, color = struct.unpack("<HHI", data[:8])
            self.framebuffer[
                (y * self.scr.width + x) * self.bitwidth : (y * self.scr.width + x + 1)
                * self.bitwidth
            ] = color.to_bytes(4, "little")[: self.bitwidth]
            self.flush()

    def flush(self):
        self.frame_update_signal.emit()

    def recv_pkt(self, data: bytes):
        """
        pack:
        0xAA 0x55 type(1) len(4) data(len)
        no end mark
        """
        self.buffer += data
        while True:
            if self.state == 0:
                idx = self.buffer.find(b"\xaa\x55")
                if idx == -1:
                    return
                self.buffer = self.buffer[idx:]
                self.state = 1
            if self.state == 1:
                if len(self.buffer) < 7:
                    return
                self.state = 2
                self.packlen = struct.unpack("<I", self.buffer[3:7])[0]
                if self.packlen == 0:
                    self.state = 0
                    self.buffer = self.buffer[7:]
                    return
            if self.state == 2:
                if len(self.buffer) < self.packlen + 7:
                    return
                self.unpack(self.buffer[2], self.buffer[7 : 7 + self.packlen])
                self.state = 0
                self.buffer = self.buffer[7 + self.packlen :]

    def _send_pkt(self, type: int, data: bytes):
        if not self.serial.isOpen():
            return
        pkt = b"\xaa\x55" + struct.pack("<BI", type, len(data)) + data
        self.serial.write(pkt)

    def _send_mouse(self):
        data = struct.pack(
            "<HHbB",
            self.mouse_state[1],
            self.mouse_state[2],
            self.mouse_state[3],
            self.mouse_state[0],
        )
        self._send_pkt(0x02, data)

    def _send_touch(self):
        data = struct.pack(
            "<HHB",
            self.touch_state[1],
            self.touch_state[2],
            int(self.touch_state[0]),
        )
        self._send_pkt(0x01, data)

    def _send_encoder(self):
        data = struct.pack(
            "<bB",
            self.encoder_state[1],
            int(self.encoder_state[0]),
        )
        self._send_pkt(0x04, data)

    def acquire_init(self):
        if (not self.serial.isOpen()) or self.scr.enable:
            return
        self._send_pkt(0xFF, b"")

    def update_mouse_key(
        self,
        btn: Qt.MouseButton,
        pressed: bool,
        x: int,
        y: int,
        in_encoder: bool = False,
    ):
        if not in_encoder:
            offset_dict = {
                Qt.MouseButton.LeftButton: 0,
                Qt.MouseButton.RightButton: 1,
                Qt.MouseButton.MiddleButton: 2,
                Qt.MouseButton.BackButton: 3,
                Qt.MouseButton.ForwardButton: 4,
                Qt.MouseButton.XButton1: 5,
                Qt.MouseButton.XButton2: 6,
            }

            if pressed:
                self.mouse_state[0] |= 1 << offset_dict[btn]
            else:
                self.mouse_state[0] &= ~(1 << offset_dict[btn])
            self.mouse_state[1] = x
            self.mouse_state[2] = y
            self.touch_state[1] = x
            self.touch_state[2] = y
            if self.mouse_enable:
                self._send_mouse()
            if self.touch_enable or self.touch_state[0]:
                if btn == Qt.MouseButton.LeftButton:
                    self.touch_state[0] = pressed
                    self._send_touch()
        elif btn == Qt.MouseButton.MiddleButton:
            self.encoder_state[0] = pressed
            if self.encoder_enable:
                self._send_encoder()

    def update_mouse_pos(self, x: int, y: int):
        self.mouse_state[1] = x
        self.mouse_state[2] = y
        self.touch_state[1] = x
        self.touch_state[2] = y
        if time.perf_counter() - self._last_pos_time > 0.02:
            if self.mouse_enable:
                self._send_mouse()
            if self.touch_enable and self.touch_state[0]:
                self._send_touch()
            self._last_pos_time = time.perf_counter()

    def update_mouse_wheel(self, delta: int, in_encoder: bool = False):
        if not in_encoder:
            if self.mouse_enable:
                self.mouse_state[3] = delta
                self._send_mouse()
                self.mouse_state[3] = 0
        elif self.encoder_enable:
            self.encoder_state[1] = delta
            self._send_encoder()
            self.encoder_state[1] = 0

    def update_keyboard(
        self, key: int, pressed: bool, repeat: bool, modifier: int, ascii: int
    ):
        if not self.keyboard_enable:
            return
        if modifier > 0xFFFF:
            modifier = 0
        data = struct.pack(
            "<BHHB",
            int(pressed) + int(repeat) * 2,
            key,
            modifier,
            ascii,
        )
        self._send_pkt(0x00, data)

    def update_button(self, idx: int, pressed: bool):
        if not self.button_enable:
            return
        data = struct.pack(
            "<BB",
            idx,
            int(pressed),
        )
        self._send_pkt(0x03, data)

    @property
    def width(self) -> int:
        return self.scr.width

    @property
    def height(self) -> int:
        return self.scr.height

    @property
    def rotation(self) -> int:
        return self.scr.rotation

    @property
    def format(self) -> int:
        return self.scr.format

    @property
    def keyboard_enable(self) -> bool:
        return bool(self.scr.indev & InDevMask.KEYBOARD)

    @property
    def touch_enable(self) -> bool:
        return bool(self.scr.indev & InDevMask.TOUCH)

    @property
    def mouse_enable(self) -> bool:
        return bool(self.scr.indev & InDevMask.MOUSE)

    @property
    def button_enable(self) -> bool:
        return bool(self.scr.indev & InDevMask.BUTTON)

    @property
    def encoder_enable(self) -> bool:
        return bool(self.scr.indev & InDevMask.ENCODER)


class MyMainWindow(QMainWindow, Ui_MainWindow):
    def __init__(self, parent=None):
        super(MyMainWindow, self).__init__(parent)
        self.setupUi(self)

        self.connected = False
        self.serial = QtSerialPort.QSerialPort()
        self.serial.setBaudRate(921600)
        self.serial.readyRead.connect(self.serial_data_handler)
        self.serial.errorOccurred.connect(self.serial_error_handler)
        self.reconnect_timer = QTimer()
        self.reconnect_timer.timeout.connect(self.try_reconnect)

        for i in range(6):
            btn = getattr(self, f"pushButton_{i}")
            btn.setText(f"B{i}")
            btn.pressed.connect(lambda _=None, idx=i: self.user_btn_pressed(idx))
            btn.released.connect(lambda _=None, idx=i: self.user_btn_released(idx))

        self.mgr = ScreenManager(self.serial)
        self.mgr.param_update_signal.connect(self.update_screen_info)
        self.mgr.frame_update_signal.connect(
            lambda: self.labelImage.setPixmap(QPixmap.fromImage(self.mgr.to_image()))
        )

        self.labelImage.setScaledContents(True)
        self.centralwidget.setMouseTracking(True)
        self.labelEncoder.setMouseTracking(True)

        self.aquire_timer = QTimer()
        self.aquire_timer.timeout.connect(self.mgr.acquire_init)
        self.aquire_timer.start(1000)

        self.hwid = None

        self.spd_counter = SpeedCounter()
        self.spd_timer = QTimer()
        self.spd_timer.timeout.connect(self.update_speed)
        self.spd_timer.start(1000)

        self.labelEncoder.setToolTip("将指针悬停在该区域, 可使用鼠标滚轮模拟编码器事件")

        self.setWindowIcon(
            self.style().standardIcon(QStyle.StandardPixmap.SP_ComputerIcon)
        )

    def serial_data_handler(self):
        data = bytes(self.serial.readAll())  # type: ignore
        self.mgr.recv_pkt(data)
        self.spd_counter.update(len(data))

    def update_speed(self):
        self.labelSpeed.setText(f"{self.spd_counter.getMbps():.1f} Mbps")

    # 断开时自动尝试重连
    def serial_error_handler(self, error: QtSerialPort.QSerialPort.SerialPortError):
        if error == QtSerialPort.QSerialPort.SerialPortError.NoError:
            return
        if self.connected:
            self.serial.close()
            self.pushButtonConnect.setText("放弃")
            self.labelInfo.setText("尝试重连")
            set_font_color(self.labelInfo, "orange")
            self.reconnect_timer.start(100)
        self.mgr.scr.enable = False
        self.update_screen_info()

    def try_reconnect(self):
        self.mgr.scr.enable = False
        if self.hwid is None:
            return
        com = get_com(self.hwid)
        if com is None:
            return
        self.serial.setPortName(com)
        if not self.serial.open(QIODeviceBase.OpenModeFlag.ReadWrite):
            return
        self.reconnect_timer.stop()
        self.pushButtonConnect.setText("断开")
        self.labelInfo.setText(f"已连接到{com}")
        set_font_color(self.labelInfo, "#77bd76")

    def user_btn_pressed(self, idx: int):
        # logger.debug(f"User btn {idx} pressed")
        self.mgr.update_button(idx, True)

    def user_btn_released(self, idx: int):
        # logger.debug(f"User btn {idx} released")
        self.mgr.update_button(idx, False)

    def update_screen_info(self):
        self.labelImage.clear()
        if not self.mgr.scr.enable:
            self.labelChannelInfo.setText("未初始化")
            return
        text = f"{self.mgr.width}x{self.mgr.height}@{self.mgr.rotation}° "
        fmt_dict = {
            Format.RGB565: "RGB16",
            Format.RGB888: "RGB24",
            Format.MONO: "MONO",
            Format.MONO_INV: "MONOi",
            Format.GRAY_8BIT: "GRAY8",
        }
        text += fmt_dict[self.mgr.format]
        text += " DEV["
        if self.mgr.keyboard_enable:
            text += "K"
        if self.mgr.touch_enable:
            text += "T"
        if self.mgr.mouse_enable:
            text += "M"
        if self.mgr.button_enable:
            text += "B"
        if self.mgr.encoder_enable:
            text += "E"
        text += "]"
        self.labelChannelInfo.setText(text)
        target_width = self.width() + self.mgr.width - self.labelImage.width()
        target_height = self.height() + self.mgr.height - self.labelImage.height()
        self.resize(target_width, target_height)
        if self.width() > target_width:
            self.resize(self.width(), int(target_height / target_width * self.width()))

    @Slot()
    def on_pushButtonConnect_clicked(self):
        if self.connected:
            if self.serial.isOpen():
                self.serial.close()
            self.reconnect_timer.stop()
            self.pushButtonConnect.setText("连接")
            self.connected = False
            self.labelInfo.setText("未连接")
            set_font_color(self.labelInfo, "white")
            self.mgr.scr.enable = False
            self.update_screen_info()
        else:
            ids = get_all_ids()
            idx = 0
            if self.hwid is not None:
                for i, id in enumerate(ids):
                    if id[0] == self.hwid:
                        idx = i
                        break
            item, ok = QInputDialog.getItem(
                self,
                "连接设备",
                "请选择设备:",
                [f"{id[2]} {id[0]}" for id in ids],
                idx,
                False,
            )
            if not ok:
                return
            self.hwid = item.split(" ")[-1]
            com = get_com(self.hwid)
            if com is None:
                QMessageBox.critical(self, "错误", "未找到设备")
                return
            self.serial.setPortName(com)
            if not self.serial.open(QIODeviceBase.OpenModeFlag.ReadWrite):
                QMessageBox.critical(self, "错误", "连接失败")
                return
            self.serial.setDataTerminalReady(True)
            self.pushButtonConnect.setText("断开")
            self.connected = True
            self.labelInfo.setText(f"已连接到{com}")
            set_font_color(self.labelInfo, "#77bd76")

    def to_image_cord(self, pos_e: QPointF) -> Tuple[bool, float, float]:
        pos_i = self.labelImage.mapFrom(self.centralwidget, pos_e)
        x, y = pos_i.x(), pos_i.y()
        w = self.labelImage.width()
        h = self.labelImage.height()
        if x < 0 or x >= w or y < 0 or y >= h:
            return False, 0, 0
        return True, x / w, y / h

    def in_encoder_cord(self, pos_e: QPointF) -> bool:
        pos_i = self.labelEncoder.mapFrom(self.centralwidget, pos_e)
        x, y = pos_i.x(), pos_i.y()
        w = self.labelEncoder.width()
        h = self.labelEncoder.height()
        if x < 0 or x >= w or y < 0 or y >= h:
            return False
        return True

    presscnt = 0

    def mousePressEvent(self, event: QMouseEvent):
        incord, xf, yf = self.to_image_cord(event.position())
        if self.in_encoder_cord(event.position()):
            self.mgr.update_mouse_key(event.button(), True, 0, 0, True)
        else:
            self.presscnt += 1
            if self.presscnt == 1:
                set_font_color(self.labelPos, "#80c37d")
            xf = max(0, min(1, xf))
            yf = max(0, min(1, yf))
            x, y = (
                round(xf * self.mgr.width),
                round(yf * self.mgr.height),
            )
            self.labelPos.setText(f"X: {x} Y: {y}")
            self.mgr.update_mouse_key(event.button(), True, x, y)
        return super().mousePressEvent(event)

    def mouseReleaseEvent(self, event: QMouseEvent):
        incord, xf, yf = self.to_image_cord(event.position())
        if self.in_encoder_cord(event.position()):
            self.mgr.update_mouse_key(event.button(), False, 0, 0, True)
        else:
            self.presscnt -= 1
            if self.presscnt <= 0:
                set_font_color(self.labelPos, "white")
                self.presscnt = 0
            xf = max(0, min(1, xf))
            yf = max(0, min(1, yf))
            x, y = (
                round(xf * self.mgr.width),
                round(yf * self.mgr.height),
            )
            self.labelPos.setText(f"X: {x} Y: {y}")
            self.mgr.update_mouse_key(event.button(), False, x, y)
        return super().mouseReleaseEvent(event)

    def mouseMoveEvent(self, event: QMouseEvent):
        incord, xf, yf = self.to_image_cord(event.position())
        if incord:
            x, y = (
                round(xf * self.mgr.width),
                round(yf * self.mgr.height),
            )
            self.labelPos.setText(f"X: {x} Y: {y}")
            self.mgr.update_mouse_pos(x, y)
        if self.in_encoder_cord(event.position()):
            set_font_color(self.labelEncoder, "#61adeb")
        else:
            set_font_color(self.labelEncoder, "white")
        return super().mouseMoveEvent(event)

    def wheelEvent(self, event: QWheelEvent):
        # logger.debug(f"Mouse Wheel: {event.angleDelta()}")
        if self.to_image_cord(event.position())[0]:
            self.mgr.update_mouse_wheel(event.angleDelta().y())
        elif self.in_encoder_cord(event.position()):
            self.mgr.update_mouse_wheel(event.angleDelta().y(), True)
        return super().wheelEvent(event)

    def keyPressEvent(self, event: QKeyEvent) -> None:
        text = event.text().encode("ascii", "ignore")
        if text:
            ascii = text[0]
        else:
            ascii = 0
        self.mgr.update_keyboard(
            event.nativeScanCode(),
            True,
            event.isAutoRepeat(),
            int(event.nativeModifiers()),
            ascii=ascii,
        )
        return super().keyPressEvent(event)

    def keyReleaseEvent(self, event: QKeyEvent) -> None:
        text = event.text().encode("ascii", "ignore")
        if text:
            ascii = text[0]
        else:
            ascii = 0
        self.mgr.update_keyboard(
            event.nativeScanCode(),
            False,
            event.isAutoRepeat(),
            int(event.nativeModifiers()),
            ascii=ascii,
        )
        return super().keyReleaseEvent(event)


def error_log(msg):
    with open("error.log", "a") as f:
        f.write(f"Error Occurred at {datetime.datetime.now()}:\n")
        f.write(f"{msg}\n")


if __name__ == "__main__":
    argv = sys.argv
    argv += ["-platform", "windows:darkmode=2", "--style", "Windows"]  # or "Fusion" ?
    app = QApplication(argv)
    myWin = MyMainWindow()
    qdarktheme.setup_theme(
        theme="dark",
        custom_colors={
            "[dark]": {
                "background>base": "#1f2021",
            }
        },
    )
    myWin.show()
    try:
        ret = app.exec()
    except Exception:
        import traceback

        error_log(traceback.format_exc())
        ret = -1
    sys.exit(ret)
