/*=============================================================================
   WARNING!!  This file is overwritten by the UIStyler each time the Styler 
   file is saved.
  
  
        Filename:  opu_cam5_dlg.h
  
        This file was generated by the NX User Interface Styler
        Created by: chudoyakoveb68
        Version: NX 4
              Date: 08-24- 7
              Time: 11:54
  
   This include file is overwritten each time the UIStyler dialog is
   saved.  Any modifications to this file will be lost.
==============================================================================*/
 
 
#ifndef OPU_CAM5_DLG_H_INCLUDED
#define OPU_CAM5_DLG_H_INCLUDED
 
#include <uf.h> 
#include <uf_defs.h>
#include <uf_styler.h> 


#ifdef __cplusplus
extern "C" {
#endif


/*------------------ UIStyler Dialog Definitions  ------------------- */
/* The following values are definitions into your UIStyler dialog.    */
/* These values will allow you to modify existing objects within your */
/* dialog.   They work directly with the NX Open API,                 */
/* UF_STYLER_ask_value, UF_STYLER_ask_values, and UF_STYLER_set_value.*/
/*------------------------------------------------------------------- */
 
#define SP_REAL_SURFACESPEED           ("REAL_SURFACESPEED")
#define SP_REAL_FEEDPERTOOTH           ("REAL_FEEDPERTOOTH")
#define SP_SEP_2                       ("SEP_2")
#define SP_TOGGLE_SPINDLESPEED         ("TOGGLE_SPINDLESPEED")
#define SP_REAL_SPINDLESPEED           ("REAL_SPINDLESPEED")
#define SP_OPTION_SPINDLEOUTPUTMODE    ("OPTION_SPINDLEOUTPUTMODE")
#define SP_SEP_6                       ("SEP_6")
#define SP_TOGGLE_RANGESTATUS          ("TOGGLE_RANGESTATUS")
#define SP_WIDE_RANGE                  ("WIDE_RANGE")
#define SP_TOGGLE_TEXTSTATUS           ("TOGGLE_TEXTSTATUS")
#define SP_WIDE_TEXT                   ("WIDE_TEXT")
#define SP_SEP_11                      ("SEP_11")
#define SP_OPTION_SPINDLEDIRECTION     ("OPTION_SPINDLEDIRECTION")
#define SP_SEP_13                      ("SEP_13")
#define SP_DIALOG_OBJECT_COUNT         ( 14 )
 

/*---------------- UIStyler Callback Prototypes --------------- */
/* The following function prototypes define the callbacks       */
/* specified in your UIStyler built dialog.  You are REQUIRED to*/
/* create the associated function for each prototype.  You must */
/* use the same function name and parameter list when creating  */
/* your callback function.                                      */
/*------------------------------------------------------------- */

int SP_apply_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data);

int SP_surface_speed_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data);

int SP_feed_per_tooth_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data);

int SP_tog_spindle_speed_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data);

int SP_spindle_speed_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data);

int SP_spindle_output_mode_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data);

int SP_range_status_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data);

int SP_range_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data);

int SP_text_status_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data);

int SP_text_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data);

int SP_spindle_direction_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data);




#ifdef __cplusplus
}
#endif



#endif /* OPU_CAM5_DLG_H_INCLUDED */