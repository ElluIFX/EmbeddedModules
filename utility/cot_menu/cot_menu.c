/**
 **********************************************************************************************************************
 * @file    cot_menu.c
 * @brief   该文件提供菜单框架功能
 * @author  const_zpc  any question please send mail to const_zpc@163.com
 * @version V1.0.0
 * @date    2023-06-21
 *
 * @details  功能详细说明：
 *           + 菜单初始化函数
 *           + 返回主菜单函数
 *           + 菜单控制函数
 *           + 菜单轮询任务函数
 *
 **********************************************************************************************************************
 * 源码路径：https://gitee.com/cot_package/cot_menu.git
 *具体问题及建议可在该网址填写 Issue
 *
 * 使用方式:
 *    1、使用前初始化函数 cotMenu_Init, 设置主菜单内容
 *    2、周期调用函数 cotMenu_Task, 用来处理菜单显示和执行相关回调函数
 *    3、使用其他函数对菜单界面控制
 *
 **********************************************************************************************************************
 */

/* Includes
 * ----------------------------------------------------------------------------------------------------------*/
#include "cot_menu.h"

#ifdef _COT_MENU_USE_MALLOC_
#include <malloc.h>
#endif

#ifdef _COT_MENU_USE_SHORTCUT_
#include <stdarg.h>
#endif

/* Private typedef
 * ---------------------------------------------------------------------------------------------------*/
typedef struct MenuCtrl {
    struct MenuCtrl* pParentMenuCtrl; /*!< 父菜单控制处理 */
    char*(pszDesc
              [COT_MENU_SUPPORT_LANGUAGE]); /*!< 当前选项的字符串描述(多语种) */
    cotShowcotMenuCallFun_f pfnShowMenuFun; /*!< 当前菜单显示效果函数 */
    cotMenuList_t* pMenuList;               /*!< 当前菜单列表 */
    cotMenuCallFun_f pfnLoadCallFun;        /*!< 当前菜单加载函数 */
    cotMenuCallFun_f pfnRunCallFun; /*!< 当前选项的非菜单功能函数 */
    menusize_t itemsNum;            /*!< 当前菜单选项总数目 */
    menusize_t showBaseItem;        /*!< 当前菜单首个显示的选项 */
    menusize_t selectItem;          /*!< 当前菜单选中的选项 */
    bool isSelected;                /*!< 菜单选项是否已经被选择 */
} MenuCtrl_t;

typedef struct {
    MenuCtrl_t* pMenuCtrl; /*!< 当前菜单控制处理 */
    cotMenuCallFun_f
        pfnMainEnterCallFun; /*!< 主菜单进入时(进入菜单)需要执行一次的函数 */
    cotMenuCallFun_f
        pfnMainExitCallFun; /*!< 主菜单进入后退出时(退出菜单)需要执行一次的函数 */
    cotMenuCallFun_f pfnLoadCallFun; /*!< 重加载函数 */
    uint8_t language;                /*!< 语种选择 */
    uint8_t isEnterMainMenu : 1;     /*!< 是否进入了主菜单 */
} MenuManage_t;

/* Private define
 * ----------------------------------------------------------------------------------------------------*/
/* Private macro
 * -----------------------------------------------------------------------------------------------------*/
/* Private variables
 * -------------------------------------------------------------------------------------------------*/
static MenuManage_t sg_tMenuManage;

#ifndef _COT_MENU_USE_MALLOC_
static MenuCtrl_t sg_arrMenuCtrl[COT_MENU_MAX_DEPTH];
#endif

static uint8_t sg_currMenuDepth = 0;

/* Private function prototypes
 * ---------------------------------------------------------------------------------------*/
static MenuCtrl_t* NewMenu(void);
static void DeleteMenu(MenuCtrl_t* pMenu);

/* Private function
 * --------------------------------------------------------------------------------------------------*/
/**
 * @brief      新建菜单层级
 *
 * @return     MenuCtrl_t*
 */
static MenuCtrl_t* NewMenu(void) {
    MenuCtrl_t* pMenuCtrl = NULL;

    if (sg_currMenuDepth < COT_MENU_MAX_DEPTH) {
#ifdef _COT_MENU_USE_MALLOC_
        pMenuCtrl = (MenuCtrl_t*)m_alloc(sizeof(MenuCtrl_t));
#else
        pMenuCtrl = &sg_arrMenuCtrl[sg_currMenuDepth];
#endif
        sg_currMenuDepth++;
    }

    return pMenuCtrl;
}

/**
 * @brief      销毁菜单层级
 *
 * @param      pMenu
 */
static void DeleteMenu(MenuCtrl_t* pMenu) {
#ifdef _COT_MENU_USE_MALLOC_
    free(pMenu);
#endif
    if (sg_currMenuDepth > 0) {
        sg_currMenuDepth--;
    }
}

/**
 * @brief      菜单初始化
 *
 * @param[in]  pMainMenu        主菜单注册信息
 * @return     0,成功; -1,失败
 */
int cotMenu_Init(cotMainMenuCfg_t* pMainMenu) {
    int i;
    MenuCtrl_t* pNewMenuCtrl = NULL;

    if (sg_tMenuManage.pMenuCtrl != NULL) {
        return -1;
    }

#if COT_MENU_MAX_DEPTH != 0
    sg_currMenuDepth = 0;
#endif

    if ((pNewMenuCtrl = NewMenu()) == NULL) {
        return -1;
    }

    sg_tMenuManage.language = 0;

    for (i = 0; i < COT_MENU_SUPPORT_LANGUAGE; i++) {
        pNewMenuCtrl->pszDesc[i] = (char*)pMainMenu->pszDesc[i];
    }

    pNewMenuCtrl->pParentMenuCtrl = NULL;
    pNewMenuCtrl->pfnLoadCallFun = pMainMenu->pfnLoadCallFun;
    pNewMenuCtrl->pfnShowMenuFun = NULL;
    pNewMenuCtrl->pfnRunCallFun = pMainMenu->pfnRunCallFun;

    pNewMenuCtrl->pMenuList = NULL;
    pNewMenuCtrl->itemsNum = 0;
    pNewMenuCtrl->selectItem = 0;
    pNewMenuCtrl->showBaseItem = 0;

    sg_tMenuManage.pMenuCtrl = pNewMenuCtrl;
    sg_tMenuManage.isEnterMainMenu = 0;
    sg_tMenuManage.pfnMainEnterCallFun = pMainMenu->pfnEnterCallFun;
    sg_tMenuManage.pfnMainExitCallFun = pMainMenu->pfnExitCallFun;
    sg_tMenuManage.pfnLoadCallFun = pNewMenuCtrl->pfnLoadCallFun;

    return 0;
}

/**
 * @brief  菜单反初始化
 *
 * @attention
 * 不管处于任何界面都会逐级退出到主菜单后（会调用退出函数），再退出主菜单，最后反初始化
 * @return 0,成功; -1,失败
 */
int cotMenu_DeInit(void) {
    if (sg_tMenuManage.pMenuCtrl == NULL) {
        return -1;
    }

    cotMenu_MainExit();

    DeleteMenu(sg_tMenuManage.pMenuCtrl);
    sg_tMenuManage.pMenuCtrl = NULL;
    sg_tMenuManage.language = 0;
    sg_tMenuManage.isEnterMainMenu = 0;
    sg_tMenuManage.pfnMainEnterCallFun = NULL;
    sg_tMenuManage.pfnMainExitCallFun = NULL;
    sg_tMenuManage.pfnLoadCallFun = NULL;

    return 0;
}

/**
 * @brief      子菜单绑定当前菜单选项
 *
 * @param      pMenuList       新的菜单列表
 * @param      menuNum         新的菜单列表数目
 * @param      pfnShowMenuFun  新的菜单列表显示效果回调函数,
 * 为NULL则延续上级菜单显示效果
 * @return     int
 */
int cotMenu_Bind(cotMenuList_t* pMenuList, menusize_t menuNum,
                 cotShowcotMenuCallFun_f pfnShowMenuFun) {
    if (sg_tMenuManage.pMenuCtrl == NULL) {
        return -1;
    }

    if (sg_tMenuManage.pMenuCtrl->pMenuList != NULL) {
        return 0;
    }

    sg_tMenuManage.pMenuCtrl->pMenuList = pMenuList;
    sg_tMenuManage.pMenuCtrl->itemsNum = menuNum;

    if (pfnShowMenuFun != NULL) {
        sg_tMenuManage.pMenuCtrl->pfnShowMenuFun = pfnShowMenuFun;
    }

    return 0;
}

/**
 * @brief      选择语种
 *
 * @param[in]  languageIdx 语种索引
 * @return     0,成功; -1,失败
 */
int cotMenu_SelectLanguage(uint8_t languageIdx) {
    if (COT_MENU_SUPPORT_LANGUAGE <= languageIdx) {
        return -1;
    }

    sg_tMenuManage.language = languageIdx;
    return 0;
}

/**
 * @brief      复位菜单, 回到主菜单界面
 *
 * @note       该复位回到主菜单不会执行退出所需要执行的回调函数
 * @return     0,成功; -1,失败
 */
int cotMenu_Reset(void) {
    if (sg_tMenuManage.pMenuCtrl == NULL ||
        sg_tMenuManage.isEnterMainMenu == 0) {
        return -1;
    }

    while (sg_tMenuManage.pMenuCtrl->pParentMenuCtrl != NULL) {
        MenuCtrl_t* pMenuCtrl = sg_tMenuManage.pMenuCtrl;

        sg_tMenuManage.pMenuCtrl = sg_tMenuManage.pMenuCtrl->pParentMenuCtrl;
        DeleteMenu(pMenuCtrl);
    }

    sg_tMenuManage.pMenuCtrl->selectItem = 0;

    return 0;
}

/**
 * @brief      主菜单进入
 *
 * @return     0,成功; -1,失败
 */
int cotMenu_MainEnter(void) {
    if (sg_tMenuManage.pMenuCtrl == NULL ||
        sg_tMenuManage.isEnterMainMenu == 1) {
        return -1;
    }

    if (sg_tMenuManage.pfnMainEnterCallFun != NULL) {
        sg_tMenuManage.pfnMainEnterCallFun();
    }

    sg_tMenuManage.isEnterMainMenu = 1;
    sg_tMenuManage.pfnLoadCallFun = sg_tMenuManage.pMenuCtrl->pfnLoadCallFun;

    return 0;
}

/**
 * @brief      主菜单退出
 *
 * @attention
 * 不管处于任何界面都会逐级退出到主菜单后（会调用退出函数），再退出主菜单
 * @return     0,成功; -1,失败
 */
int cotMenu_MainExit(void) {
    if (sg_tMenuManage.pMenuCtrl == NULL ||
        sg_tMenuManage.isEnterMainMenu == 0) {
        return -1;
    }

    while (cotMenu_Exit(1) == 0) {}

    if (sg_tMenuManage.pfnMainExitCallFun != NULL) {
        sg_tMenuManage.pfnMainExitCallFun();
    }

    sg_tMenuManage.isEnterMainMenu = 0;

    return 0;
}

/**
 * @brief      进入当前菜单选项
 *
 * @return     0,成功; -1,失败
 */
int cotMenu_Enter(void) {
    int i;
    MenuCtrl_t* pNewMenuCtrl = NULL;
    MenuCtrl_t* pCurrMenuCtrl = sg_tMenuManage.pMenuCtrl;

    if (sg_tMenuManage.pMenuCtrl == NULL ||
        sg_tMenuManage.isEnterMainMenu == 0) {
        return -1;
    }

    if ((pNewMenuCtrl = NewMenu()) == NULL) {
        return -1;
    }

    for (i = 0; i < COT_MENU_SUPPORT_LANGUAGE; i++) {
        pNewMenuCtrl->pszDesc[i] =
            (char*)pCurrMenuCtrl->pMenuList[pCurrMenuCtrl->selectItem]
                .pszDesc[i];
    }

    pNewMenuCtrl->pMenuList = NULL;
    pNewMenuCtrl->itemsNum = 0;
    pNewMenuCtrl->pfnShowMenuFun = pCurrMenuCtrl->pfnShowMenuFun;
    pNewMenuCtrl->pfnLoadCallFun =
        pCurrMenuCtrl->pMenuList[pCurrMenuCtrl->selectItem].pfnLoadCallFun;
    pNewMenuCtrl->pfnRunCallFun =
        pCurrMenuCtrl->pMenuList[pCurrMenuCtrl->selectItem].pfnRunCallFun;
    pNewMenuCtrl->selectItem = 0;
    pNewMenuCtrl->isSelected = true;
    pNewMenuCtrl->pParentMenuCtrl = pCurrMenuCtrl;

    sg_tMenuManage.pMenuCtrl = pNewMenuCtrl;
    sg_tMenuManage.pfnLoadCallFun = pNewMenuCtrl->pfnLoadCallFun;

    if (pCurrMenuCtrl->pMenuList[pCurrMenuCtrl->selectItem].pfnEnterCallFun !=
        NULL) {
        pCurrMenuCtrl->pMenuList[pCurrMenuCtrl->selectItem].pfnEnterCallFun();
    }

    return 0;
}

/**
 * @brief      退出当前选项并返回上一层菜单
 *
 * @param[in]  isReset 菜单选项是否从头选择
 * @return     0,成功; -1,失败, 即目前处于主菜单, 无法返回
 */
int cotMenu_Exit(bool isReset) {
    MenuCtrl_t* pMenuCtrl = sg_tMenuManage.pMenuCtrl;

    if (sg_tMenuManage.pMenuCtrl == NULL ||
        sg_tMenuManage.isEnterMainMenu == 0) {
        return -1;
    }

    if (sg_tMenuManage.pMenuCtrl->pParentMenuCtrl == NULL) {
        return -1;
    }

    sg_tMenuManage.pMenuCtrl = sg_tMenuManage.pMenuCtrl->pParentMenuCtrl;
    sg_tMenuManage.pfnLoadCallFun = sg_tMenuManage.pMenuCtrl->pfnLoadCallFun;
    DeleteMenu(pMenuCtrl);
    pMenuCtrl = NULL;

    if (sg_tMenuManage.pMenuCtrl
            ->pMenuList[sg_tMenuManage.pMenuCtrl->selectItem]
            .pfnExitCallFun != NULL) {
        sg_tMenuManage.pMenuCtrl->isSelected = false;
        sg_tMenuManage.pMenuCtrl
            ->pMenuList[sg_tMenuManage.pMenuCtrl->selectItem]
            .pfnExitCallFun();
    }

    if (isReset) {
        sg_tMenuManage.pMenuCtrl->selectItem = 0;
    }

    return 0;
}

/**
 * @brief      选择上一个菜单选项
 *
 * @param[in]  isAllowRoll 第一个选项时是否从跳转到最后一个选项
 * @return     0,成功; -1,失败
 */
int cotMenu_SelectPrevious(bool isAllowRoll) {
    if (sg_tMenuManage.pMenuCtrl == NULL ||
        sg_tMenuManage.isEnterMainMenu == 0) {
        return -1;
    }

    if (sg_tMenuManage.pMenuCtrl->selectItem > 0) {
        sg_tMenuManage.pMenuCtrl->selectItem--;
    } else {
        if (isAllowRoll) {
            sg_tMenuManage.pMenuCtrl->selectItem =
                sg_tMenuManage.pMenuCtrl->itemsNum - 1;
        } else {
            sg_tMenuManage.pMenuCtrl->selectItem = 0;
            return -1;
        }
    }

    return 0;
}

/**
 * @brief      选择下一个菜单选项
 *
 * @param[in]  isAllowRoll 最后一个选项时是否跳转到第一个选项
 * @return     0,成功; -1,失败
 */
int cotMenu_SelectNext(bool isAllowRoll) {
    if (sg_tMenuManage.pMenuCtrl == NULL ||
        sg_tMenuManage.isEnterMainMenu == 0) {
        return -1;
    }

    if (sg_tMenuManage.pMenuCtrl->selectItem <
        (sg_tMenuManage.pMenuCtrl->itemsNum - 1)) {
        sg_tMenuManage.pMenuCtrl->selectItem++;
    } else {
        if (isAllowRoll) {
            sg_tMenuManage.pMenuCtrl->selectItem = 0;
        } else {
            sg_tMenuManage.pMenuCtrl->selectItem =
                sg_tMenuManage.pMenuCtrl->itemsNum - 1;
            return -1;
        }
    }

    return 0;
}

#ifdef _COT_MENU_USE_SHORTCUT_

/**
 * @brief      相对主菜单或当前菜单通过下级各菜单索引快速进入指定选项
 *
 * @param[in]  isAbsolute 是否采用绝对菜单索引（从主菜单开始）
 * @param[in]  deep 菜单深度，大于 0
 * @param[in]  ...  各级菜单索引值(从0开始), 入参个数由 deep 的值决定
 * @return     0,成功; -1,失败
 */
int cotMenu_ShortcutEnter(bool isAbsolute, int deep, ...) {
    uint8_t selectDeep = 0;
    va_list pItemList;
    menusize_t selectItem;

    if (sg_tMenuManage.pMenuCtrl == NULL ||
        sg_tMenuManage.isEnterMainMenu == 0) {
        return -1;
    }

    if (isAbsolute) {
        cotMenu_Reset();
    }

    va_start(pItemList, deep);

    while (selectDeep < deep) {
        selectItem = va_arg(pItemList, int);

        if (selectItem >= sg_tMenuManage.pMenuCtrl->itemsNum) {
            va_end(pItemList);
            return -1;
        }

        sg_tMenuManage.pMenuCtrl->selectItem = selectItem;
        cotMenu_Enter();
        selectDeep++;
    }

    va_end(pItemList);

    return 0;
}

#endif

/**
 * @brief      限制当前菜单界面最多显示的菜单数目
 *
 * @note       在菜单显示效果回调函数中使用, 使用成员变量 showBaseItem
 * 得到显示界面的第一个选项索引
 * @param[in,out]  tMenuShow   当前菜单显示信息
 * @param[in,out]  showNum     当前菜单中需要显示的选项数目,
 * 根据当前菜单选项的总数得到最终的显示的选项数目
 * @return     0,成功; -1,失败
 */
int cotMenu_LimitShowListNum(cotMenuShow_t* ptMenuShow, menusize_t* pShowNum) {
    if (ptMenuShow == NULL || pShowNum == NULL) {
        return -1;
    }

    if (*pShowNum > ptMenuShow->itemsNum) {
        *pShowNum = ptMenuShow->itemsNum;
    }

    if (ptMenuShow->selectItem < ptMenuShow->showBaseItem) {
        ptMenuShow->showBaseItem = ptMenuShow->selectItem;
    } else if ((ptMenuShow->selectItem - ptMenuShow->showBaseItem) >=
               *pShowNum) {
        ptMenuShow->showBaseItem = ptMenuShow->selectItem - *pShowNum + 1;
    } else {
        // 保持
    }

    return 0;
}

/**
 * @brief       获取当前父菜单显示信息
 *              如获取当前菜单的二级父菜单信息，level 为2
 *
 * @param[out]  ptMenuShow 父 n 级菜单显示信息
 * @param[in]   level      n 级, 大于 0
 * @return int
 */
int cotMenu_QueryParentMenu(cotMenuShow_t* ptMenuShow, uint8_t level) {
    int i;
    cotMenuList_t* pMenu;
    MenuCtrl_t* pMenuCtrl = NULL;

    if (sg_tMenuManage.pMenuCtrl == NULL ||
        sg_tMenuManage.isEnterMainMenu == 0) {
        return -1;
    }

    pMenuCtrl = sg_tMenuManage.pMenuCtrl->pParentMenuCtrl;

    while (level && pMenuCtrl != NULL) {
        pMenu = pMenuCtrl->pMenuList;
        ptMenuShow->itemsNum = pMenuCtrl->itemsNum;
        ptMenuShow->selectItem = pMenuCtrl->selectItem;
        ptMenuShow->showBaseItem = pMenuCtrl->showBaseItem;

        ptMenuShow->pszDesc =
            sg_tMenuManage.pMenuCtrl->pszDesc[sg_tMenuManage.language];

        for (i = 0; i < ptMenuShow->itemsNum && i < COT_MENU_MAX_NUM; i++) {
            ptMenuShow->pszItemsDesc[i] =
                (char*)pMenu[i].pszDesc[sg_tMenuManage.language];
            ptMenuShow->pItemsExData[i] = pMenu[i].pExtendData;
        }

        pMenuCtrl = pMenuCtrl->pParentMenuCtrl;
        level--;
    }

    if (level != 0 && pMenuCtrl == NULL) {
        return -1;
    }

    return 0;
}

/**
 * @brief  菜单任务
 *
 * @return 0,成功, 处于菜单模式下; -1,失败, 未处于菜单模式下
 */
int cotMenu_Task(void) {
    int i;
    cotMenuList_t* pMenuList;
    cotMenuShow_t tMenuShow;

    if (sg_tMenuManage.pMenuCtrl == NULL ||
        sg_tMenuManage.isEnterMainMenu == 0) {
        return -1;
    }

    if (sg_tMenuManage.pfnLoadCallFun != NULL) {
        sg_tMenuManage.pfnLoadCallFun();
        sg_tMenuManage.pfnLoadCallFun = NULL;
    }

    if (sg_tMenuManage.pMenuCtrl->pMenuList != NULL) {
        pMenuList = sg_tMenuManage.pMenuCtrl->pMenuList;
        tMenuShow.itemsNum = sg_tMenuManage.pMenuCtrl->itemsNum;
        tMenuShow.selectItem = sg_tMenuManage.pMenuCtrl->selectItem;
        tMenuShow.showBaseItem = sg_tMenuManage.pMenuCtrl->showBaseItem;

        tMenuShow.pszDesc =
            sg_tMenuManage.pMenuCtrl->pszDesc[sg_tMenuManage.language];

        for (i = 0; i < tMenuShow.itemsNum && i < COT_MENU_MAX_NUM; i++) {
            tMenuShow.pszItemsDesc[i] =
                (char*)pMenuList[i].pszDesc[sg_tMenuManage.language];
            tMenuShow.pItemsExData[i] = pMenuList[i].pExtendData;
        }

        if (sg_tMenuManage.pMenuCtrl->pfnShowMenuFun != NULL) {
            sg_tMenuManage.pMenuCtrl->pfnShowMenuFun(&tMenuShow);
        }

        sg_tMenuManage.pMenuCtrl->showBaseItem = tMenuShow.showBaseItem;
    }

    if (sg_tMenuManage.pMenuCtrl->pfnRunCallFun != NULL) {
        sg_tMenuManage.pMenuCtrl->pfnRunCallFun();
    }

    return 0;
}
