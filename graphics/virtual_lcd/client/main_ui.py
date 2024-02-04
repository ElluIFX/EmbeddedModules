# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'main.ui'
##
## Created by: Qt User Interface Compiler version 6.5.3
##
## WARNING! All changes made in this file will be lost when recompiling UI file!
################################################################################

from PySide6.QtCore import (QCoreApplication, QDate, QDateTime, QLocale,
    QMetaObject, QObject, QPoint, QRect,
    QSize, QTime, QUrl, Qt)
from PySide6.QtGui import (QBrush, QColor, QConicalGradient, QCursor,
    QFont, QFontDatabase, QGradient, QIcon,
    QImage, QKeySequence, QLinearGradient, QPainter,
    QPalette, QPixmap, QRadialGradient, QTransform)
from PySide6.QtWidgets import (QApplication, QFrame, QHBoxLayout, QLabel,
    QMainWindow, QPushButton, QSizePolicy, QSpacerItem,
    QVBoxLayout, QWidget)

class Ui_MainWindow(object):
    def setupUi(self, MainWindow):
        if not MainWindow.objectName():
            MainWindow.setObjectName(u"MainWindow")
        MainWindow.resize(800, 600)
        MainWindow.setMouseTracking(True)
        MainWindow.setTabletTracking(True)
        self.centralwidget = QWidget(MainWindow)
        self.centralwidget.setObjectName(u"centralwidget")
        self.verticalLayout_2 = QVBoxLayout(self.centralwidget)
        self.verticalLayout_2.setObjectName(u"verticalLayout_2")
        self.frame = QFrame(self.centralwidget)
        self.frame.setObjectName(u"frame")
        self.frame.setMouseTracking(True)
        self.frame.setTabletTracking(True)
        self.frame.setFrameShape(QFrame.StyledPanel)
        self.frame.setFrameShadow(QFrame.Plain)
        self.verticalLayout = QVBoxLayout(self.frame)
        self.verticalLayout.setObjectName(u"verticalLayout")
        self.labelImage = QLabel(self.frame)
        self.labelImage.setObjectName(u"labelImage")
        self.labelImage.setMouseTracking(True)
        self.labelImage.setTabletTracking(True)

        self.verticalLayout.addWidget(self.labelImage)


        self.verticalLayout_2.addWidget(self.frame)

        self.horizontalLayout_5 = QHBoxLayout()
        self.horizontalLayout_5.setObjectName(u"horizontalLayout_5")
        self.labelPos = QLabel(self.centralwidget)
        self.labelPos.setObjectName(u"labelPos")
        self.labelPos.setMinimumSize(QSize(100, 0))
        self.labelPos.setFrameShape(QFrame.Box)
        self.labelPos.setAlignment(Qt.AlignCenter)

        self.horizontalLayout_5.addWidget(self.labelPos)

        self.horizontalSpacer_3 = QSpacerItem(40, 20, QSizePolicy.Expanding, QSizePolicy.Minimum)

        self.horizontalLayout_5.addItem(self.horizontalSpacer_3)

        self.pushButton_0 = QPushButton(self.centralwidget)
        self.pushButton_0.setObjectName(u"pushButton_0")
        self.pushButton_0.setMaximumSize(QSize(60, 16777215))
        self.pushButton_0.setFocusPolicy(Qt.NoFocus)

        self.horizontalLayout_5.addWidget(self.pushButton_0)

        self.pushButton_1 = QPushButton(self.centralwidget)
        self.pushButton_1.setObjectName(u"pushButton_1")
        self.pushButton_1.setMaximumSize(QSize(60, 16777215))
        self.pushButton_1.setFocusPolicy(Qt.NoFocus)

        self.horizontalLayout_5.addWidget(self.pushButton_1)

        self.pushButton_2 = QPushButton(self.centralwidget)
        self.pushButton_2.setObjectName(u"pushButton_2")
        self.pushButton_2.setMaximumSize(QSize(60, 16777215))
        self.pushButton_2.setFocusPolicy(Qt.NoFocus)

        self.horizontalLayout_5.addWidget(self.pushButton_2)

        self.pushButton_3 = QPushButton(self.centralwidget)
        self.pushButton_3.setObjectName(u"pushButton_3")
        self.pushButton_3.setMaximumSize(QSize(60, 16777215))
        self.pushButton_3.setFocusPolicy(Qt.NoFocus)

        self.horizontalLayout_5.addWidget(self.pushButton_3)

        self.pushButton_4 = QPushButton(self.centralwidget)
        self.pushButton_4.setObjectName(u"pushButton_4")
        self.pushButton_4.setMaximumSize(QSize(60, 16777215))
        self.pushButton_4.setFocusPolicy(Qt.NoFocus)

        self.horizontalLayout_5.addWidget(self.pushButton_4)

        self.pushButton_5 = QPushButton(self.centralwidget)
        self.pushButton_5.setObjectName(u"pushButton_5")
        self.pushButton_5.setMaximumSize(QSize(60, 16777215))
        self.pushButton_5.setFocusPolicy(Qt.NoFocus)

        self.horizontalLayout_5.addWidget(self.pushButton_5)

        self.horizontalSpacer_2 = QSpacerItem(40, 20, QSizePolicy.Expanding, QSizePolicy.Minimum)

        self.horizontalLayout_5.addItem(self.horizontalSpacer_2)

        self.labelEncoder = QLabel(self.centralwidget)
        self.labelEncoder.setObjectName(u"labelEncoder")
        self.labelEncoder.setMinimumSize(QSize(100, 0))
        self.labelEncoder.setFrameShape(QFrame.Box)
        self.labelEncoder.setAlignment(Qt.AlignCenter)

        self.horizontalLayout_5.addWidget(self.labelEncoder)


        self.verticalLayout_2.addLayout(self.horizontalLayout_5)

        self.horizontalLayout_4 = QHBoxLayout()
        self.horizontalLayout_4.setObjectName(u"horizontalLayout_4")
        self.labelChannelInfo = QLabel(self.centralwidget)
        self.labelChannelInfo.setObjectName(u"labelChannelInfo")
        self.labelChannelInfo.setAlignment(Qt.AlignCenter)

        self.horizontalLayout_4.addWidget(self.labelChannelInfo)

        self.labelSpeed = QLabel(self.centralwidget)
        self.labelSpeed.setObjectName(u"labelSpeed")
        self.labelSpeed.setAlignment(Qt.AlignCenter)

        self.horizontalLayout_4.addWidget(self.labelSpeed)

        self.horizontalSpacer = QSpacerItem(40, 20, QSizePolicy.Expanding, QSizePolicy.Minimum)

        self.horizontalLayout_4.addItem(self.horizontalSpacer)

        self.label = QLabel(self.centralwidget)
        self.label.setObjectName(u"label")
        self.label.setAlignment(Qt.AlignCenter)

        self.horizontalLayout_4.addWidget(self.label)

        self.labelInfo = QLabel(self.centralwidget)
        self.labelInfo.setObjectName(u"labelInfo")

        self.horizontalLayout_4.addWidget(self.labelInfo)

        self.pushButtonConnect = QPushButton(self.centralwidget)
        self.pushButtonConnect.setObjectName(u"pushButtonConnect")
        self.pushButtonConnect.setFocusPolicy(Qt.NoFocus)

        self.horizontalLayout_4.addWidget(self.pushButtonConnect)


        self.verticalLayout_2.addLayout(self.horizontalLayout_4)

        self.verticalLayout_2.setStretch(0, 1)
        MainWindow.setCentralWidget(self.centralwidget)

        self.retranslateUi(MainWindow)

        QMetaObject.connectSlotsByName(MainWindow)
    # setupUi

    def retranslateUi(self, MainWindow):
        MainWindow.setWindowTitle(QCoreApplication.translate("MainWindow", u"Virtual LCD", None))
        self.labelImage.setText("")
        self.labelPos.setText(QCoreApplication.translate("MainWindow", u"X: 0, Y: 0", None))
        self.pushButton_0.setText("")
        self.pushButton_1.setText("")
        self.pushButton_2.setText("")
        self.pushButton_3.setText("")
        self.pushButton_4.setText("")
        self.pushButton_5.setText("")
        self.labelEncoder.setText(QCoreApplication.translate("MainWindow", u"Encoder Zone", None))
        self.labelChannelInfo.setText(QCoreApplication.translate("MainWindow", u"\u672a\u521d\u59cb\u5316", None))
        self.labelSpeed.setText("")
        self.label.setText(QCoreApplication.translate("MainWindow", u"\u72b6\u6001:", None))
        self.labelInfo.setText(QCoreApplication.translate("MainWindow", u"\u672a\u8fde\u63a5", None))
        self.pushButtonConnect.setText(QCoreApplication.translate("MainWindow", u"\u8fde\u63a5", None))
    # retranslateUi

