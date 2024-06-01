#ifndef __UGUI_CONFIG_H
#define __UGUI_CONFIG_H

/* --------------------------------------------------------------------------------
 */
/* -- CONFIG SECTION -- */
/* --------------------------------------------------------------------------------
 */

#include "modules.h"
/* Specify platform-dependent integer types here */

#define __UG_FONT_DATA const
typedef uint8_t UG_U8;
typedef int8_t UG_S8;
typedef uint16_t UG_U16;
typedef int16_t UG_S16;
typedef uint32_t UG_U32;
typedef int32_t UG_S32;

/* --------------------------------------------------------------------------------
 */
/* --------------------------------------------------------------------------------
 */

/* Feature enablers */
#define USE_PRERENDER_EVENT
#define USE_POSTRENDER_EVENT

#endif
