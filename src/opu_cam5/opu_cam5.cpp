//////////////////////////////////////////////////////////////////////////////
//
//  opu_cam5.cpp
//
//  Description:
//      Contains Unigraphics entry points for the application.
//
//////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_DEPRECATE 1

/*  Include files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

//  Include files
#include <uf.h>
#include <uf_exit.h>
#include <uf_ui.h>
#include <uf_defs.h>
#include <uf_styler.h>
#include <uf_mb.h>

#include <uf_obj.h>
#include <uf_object_types.h>
#include <uf_disp.h>
#include <uf_part.h>

#include <uf_setup.h>
#include <uf_ncgroup.h>
#include <uf_oper.h>
#include <uf_ui_ont.h>
#include <uf_param.h>
#include <uf_ugopenint.h>
#include <uf_param_indices.h>

/*
#if ! defined ( __hp9000s800 ) && ! defined ( __sgi ) && ! defined ( __sun )
# include <strstream>
  using std::ostrstream;
  using std::endl;
  using std::ends;
#else
# include <strstream.h>
#endif
#include <iostream.h>
*/

#include "opu_cam5.h"

#include "opu_cam5_dlg.h"

/* The following definition defines the number of callback entries */
/* in the callback structure:                                      */
/* UF_STYLER_callback_info_t SP_cbs */
#define SP_CB_COUNT ( 14 + 1 ) /* Add 1 for the terminator */

/*--------------------------------------------------------------------------
The following structure defines the callback entries used by the
styler file.  This structure MUST be passed into the user function,
UF_STYLER_create_dialog along with SP_CB_COUNT.
--------------------------------------------------------------------------*/
static UF_STYLER_callback_info_t SP_cbs[SP_CB_COUNT] =
{
 {UF_STYLER_DIALOG_INDEX, UF_STYLER_CONSTRUCTOR_CB  , 0, SP_construct_cb},
 {UF_STYLER_DIALOG_INDEX, UF_STYLER_APPLY_CB        , 0, SP_apply_cb},
 {SP_ACTION_CALC        , UF_STYLER_ACTIVATE_CB     , 1, SP_action_calc_act_cb},
 {SP_RADIO_CALC         , UF_STYLER_VALUE_CHANGED_CB, 0, SP_radio_calc_cb},
 {SP_REAL_SURFACESPEED  , UF_STYLER_ACTIVATE_CB     , 0, SP_surface_speed_cb},
 {SP_REAL_FEEDPERTOOTH  , UF_STYLER_ACTIVATE_CB     , 0, SP_feed_per_tooth_cb},
 {SP_TOGGLE_SPINDLESPEED, UF_STYLER_VALUE_CHANGED_CB, 0, SP_tog_spindle_speed_cb},
 {SP_REAL_SPINDLESPEED  , UF_STYLER_ACTIVATE_CB     , 0, SP_spindle_speed_cb},
 {SP_OPTION_SPINDLEOUTPUTMODE, UF_STYLER_ACTIVATE_CB     , 0, SP_spindle_output_mode_cb},
 {SP_TOGGLE_RANGESTATUS , UF_STYLER_VALUE_CHANGED_CB, 0, SP_range_status_cb},
 {SP_WIDE_RANGE         , UF_STYLER_ACTIVATE_CB     , 0, SP_range_cb},
 {SP_TOGGLE_TEXTSTATUS  , UF_STYLER_VALUE_CHANGED_CB, 0, SP_text_status_cb},
 {SP_WIDE_TEXT          , UF_STYLER_ACTIVATE_CB     , 0, SP_text_cb},
 {SP_OPTION_SPINDLEDIRECTION, UF_STYLER_ACTIVATE_CB     , 0, SP_spindle_direction_cb},
 {UF_STYLER_NULL_OBJECT, UF_STYLER_NO_CB, 0, 0 }
};



/*--------------------------------------------------------------------------
UF_MB_styler_actions_t contains 4 fields.  These are defined as follows:

Field 1 : the name of your dialog that you wish to display.
Field 2 : any client data you wish to pass to your callbacks.
Field 3 : your callback structure.
Field 4 : flag to inform menubar of your dialog location.  This flag MUST
          match the resource set in your dialog!  Do NOT ASSUME that changing
          this field will update the location of your dialog.  Please use the
          UIStyler to indicate the position of your dialog.
--------------------------------------------------------------------------*/
static UF_MB_styler_actions_t actions[] = {
    { "opu_cam5_dlg.dlg",  NULL,   SP_cbs,  UF_MB_STYLER_IS_NOT_TOP },
    { NULL,  NULL,  NULL,  0 } /* This is a NULL terminated list */
};




/*****************************************************************************
**  Activation Methods
*****************************************************************************/

static void PrintErrorMessage( int errorCode ) ;


#define UF_CALL(X) (report( __FILE__, __LINE__, #X, (X)))

static int report( char *file, int line, char *call, int irc)
{
  if (irc)
  {
     char    messg[133];
     printf("%s, line %d:  %s\n", file, line, call);
     (UF_get_fail_message(irc, messg)) ?
       printf("    returned a %d\n", irc) :
       printf("    returned error %d:  %s\n", irc, messg);
  }
  return(irc);
}


#define COUNT_PRG 300

struct PRG
{
   int     num;
   tag_t   tag;
   char    name[UF_OPER_MAX_NAME_LEN+1];
} ;

static struct PRG prg_list[COUNT_PRG] ;
int prg_list_count=0;

struct SPINDLE {
	double surface_speed ;
	double feed_per_tooth ;
	int    tog_spindle_rpm ;
	double spindle_rpm ;
	int    spindle_mode ;
	int    tog_spindle_range ;
	char   spindle_range[133] ;
	int    tog_spindle_text ;
	char   spindle_text[133] ;
	int    spindle_direction_control ;
} ;

static struct SPINDLE sp ;

int _run ( tag_t prg , int calc) ;
int cam5_ask( tag_t prg ) ;
static logical cycleGeneratePrg( tag_t   tag, void   *data );
int _main_loadDll( void );
int cam5( void ) ;

int _construct_cb ( int dialog_id );
int _apply_cb ( int dialog_id ) ;
int _tog_spindle_speed_cb ( int dialog_id ) ;
int _spindle_speed_cb ( int dialog_id ) ;
int _range_status_cb ( int dialog_id ) ;
int _text_status_cb ( int dialog_id ) ;
int _action_calc_act_cb ( int dialog_id ) ;
int _radio_calc_cb ( int dialog_id );

/*    */
/*----------------------------------------------------------------------------*/
// calc=0  - вычисление режимов реза из операций (читаем D-(диаметр) z(количество зубьев) F(подача реза)
// calc!=0 - подача на зуб и скорость реза - заноситься как есть
int _run ( tag_t prg , int calc)
{
  tag_t  tls = null_tag ;
  int    tool_flutes_number = 0 ;
  double tool_diameter = 0 ;
  UF_PARAM_feedrate_t  feed;
  double smm = 0, mm=0 ;
  double surface_speed , feed_per_tooth ;

  char  prog_name[UF_OPER_MAX_NAME_LEN+1];
  int    ret = 0 ;
  int   type, subtype;
  UF_PARAM_spindle_mode_t spindle_mode ;
  UF_PARAM_spindle_dir_control_t spindle_dir_control ;

  ret = 0 ;

  /* type =               subtype =
  //     программа=121              160
  //     операция =100              110 */
  UF_CALL( UF_OBJ_ask_type_and_subtype (prg, &type, &subtype ) );
  printf(" type=%d subtype=%d ",type,subtype);
  if (type!=UF_machining_operation_type) { return (ret); }

  prog_name[0]='\0';
  //UF_OBJ_ask_name(prg, prog_name);// спросим имя обьекта
  UF_CALL( UF_OPER_ask_name_from_tag(prg, prog_name) );
  printf(" oper=%s \n",prog_name);

  surface_speed = sp.surface_speed ;
  feed_per_tooth = sp.feed_per_tooth ;

  //
  printf(" calc=%d  ",calc);

  UF_CALL( UF_OPER_ask_cutter_group(prg,&tls) );

  if (calc==0) {
  	if (tls!=null_tag) {
      UF_PARAM_ask_double_value(tls,UF_PARAM_TL_DIAMETER,&tool_diameter);
      UF_PARAM_ask_int_value(tls,UF_PARAM_TL_NUM_FLUTES,&tool_flutes_number);
      UF_PARAM_ask_subobj_ptr_value (prg,UF_PARAM_FEED_CUT,&feed);
      if (feed.value<=0.0) feed.value=0.0;
      smm = ( tool_diameter * 3.1415926 * sp.spindle_rpm ) / 1000.0 ;
      if (tool_flutes_number>0 && sp.spindle_rpm>0)
         mm = feed.value / ( tool_flutes_number * sp.spindle_rpm ) ;
      surface_speed = smm ;
      feed_per_tooth = mm ;
    }
  }

  printf("\tD=%.2f,z=%d,F=%.3f,smm=%.3f,mm=%.4f\n",tool_diameter,tool_flutes_number,feed.value,surface_speed,feed_per_tooth);

  UF_CALL( UF_PARAM_set_double_value(prg,UF_PARAM_SURFACE_SPEED,-9999.999 ) );
  UF_CALL( UF_PARAM_set_double_value(prg,UF_PARAM_SURFACE_SPEED,surface_speed ) );
  UF_CALL( UF_PARAM_set_double_value(prg,UF_PARAM_FEED_PER_TOOTH,-9999.999 ) );
  UF_CALL( UF_PARAM_set_double_value(prg,UF_PARAM_FEED_PER_TOOTH,feed_per_tooth ) );

  UF_CALL( UF_PARAM_set_double_value(prg,UF_PARAM_SPINDLE_RPM,-9999.999 ) );
  UF_CALL( UF_PARAM_set_double_value(prg,UF_PARAM_SPINDLE_RPM,sp.spindle_rpm ) );

  UF_CALL( UF_PARAM_set_int_value(prg,UF_PARAM_SPINDLE_RPM_TOG,-9999) );
  UF_CALL( UF_PARAM_set_int_value(prg,UF_PARAM_SPINDLE_RPM_TOG,sp.tog_spindle_rpm) );
  if (sp.tog_spindle_rpm==0) {
    UF_CALL( UF_PARAM_set_double_value(prg,UF_PARAM_SURFACE_SPEED,0.0 ) );
    UF_CALL( UF_PARAM_set_double_value(prg,UF_PARAM_FEED_PER_TOOTH,0.0 ) );
    UF_CALL( UF_PARAM_set_double_value(prg,UF_PARAM_SPINDLE_RPM,0.0 ) );
  }

  spindle_mode=UF_PARAM_spindle_mode_rpm ;
  if (sp.spindle_mode==0) { spindle_mode=UF_PARAM_spindle_mode_rpm; }
  if (sp.spindle_mode==1) { spindle_mode=UF_PARAM_spindle_mode_css; }
  if (sp.spindle_mode==2) { spindle_mode=UF_PARAM_spindle_mode_css; }
  if (sp.spindle_mode==3) { spindle_mode=UF_PARAM_spindle_mode_none; }

  UF_CALL( UF_PARAM_set_int_value(prg,UF_PARAM_SPINDLE_MODE,-9999) );
  UF_CALL( UF_PARAM_set_int_value(prg,UF_PARAM_SPINDLE_MODE,spindle_mode) );

  UF_CALL( UF_PARAM_set_str_value(prg,UF_PARAM_SPINDLE_RANGE_TEXT,sp.spindle_range) );

  UF_CALL( UF_PARAM_set_str_value(prg,UF_PARAM_SPINDLE_TEXT,sp.spindle_text) );

  spindle_dir_control=UF_PARAM_spindle_dir_control_clw;
  if (sp.spindle_direction_control==0) { spindle_dir_control=UF_PARAM_spindle_dir_control_none; }
  if (sp.spindle_direction_control==2) { spindle_dir_control=UF_PARAM_spindle_dir_control_cclw; }

  if (tls!=null_tag) {
    UF_CALL( UF_PARAM_set_int_value(tls,UF_PARAM_TL_DIAMETER,spindle_dir_control) );
  }

  ret = 1;

  return (ret);
}



int cam5_ask( tag_t prg )
{
  char  str1[133],str2[133] ;
  tag_t tls ;

  UF_CALL( UF_PARAM_ask_double_value(prg,UF_PARAM_SURFACE_SPEED,&sp.surface_speed ) );
  UF_CALL( UF_PARAM_ask_double_value(prg,UF_PARAM_FEED_PER_TOOTH,&sp.feed_per_tooth ) );
  UF_CALL( UF_PARAM_ask_double_value(prg,UF_PARAM_SPINDLE_RPM,&sp.spindle_rpm ) );
  UF_CALL( UF_PARAM_ask_int_value(prg,UF_PARAM_SPINDLE_RPM_TOG,&sp.tog_spindle_rpm) );

  UF_CALL( UF_PARAM_ask_int_value(prg,UF_PARAM_SPINDLE_MODE,&sp.spindle_mode) );

  str1[0]='\0';
  UF_CALL( UF_PARAM_ask_str_value(prg,UF_PARAM_SPINDLE_RANGE_TEXT,str1) );
  sp.spindle_range[0]='\0'; sprintf(sp.spindle_range,"%s",str1);
  //UF_CALL( UF_PARAM_ask_int_value(prg,UF_PARAM_SPINDLE_RANGE_TEXT_TOG,&sp.tog_spindle_range) );
  sp.tog_spindle_range=0;
  if (strlen(sp.spindle_range)>0) sp.tog_spindle_range=1;

  str2[0]='\0';
  UF_CALL( UF_PARAM_ask_str_value(prg,UF_PARAM_SPINDLE_TEXT,str2) );
  sp.spindle_text[0]='\0'; sprintf(sp.spindle_text,"%s",str2);
  //UF_CALL( UF_PARAM_ask_int_value(prg,UF_PARAM_SPINDLE_TEXT_TOG,&sp.tog_spindle_text) );
  sp.tog_spindle_text=0;
  if (strlen(sp.spindle_text)>0) sp.tog_spindle_text=1;

  UF_CALL( UF_OPER_ask_cutter_group(prg,&tls) );
  if (tls!=null_tag) {
   UF_CALL( UF_PARAM_ask_int_value(tls,UF_PARAM_TL_DIRECTION,&sp.spindle_direction_control) );
  }

  return(0);
}


/* */
/*----------------------------------------------------------------------------*/
static logical cycleGeneratePrg( tag_t   tag, void   *data )
{
   char     name[UF_OPER_MAX_NAME_LEN + 1];
   int      ecode;

   name[0]='\0';
   ecode = UF_OBJ_ask_name(tag, name);// спросим имя обьекта
   //UF_UI_write_listing_window("\n");  UF_UI_write_listing_window(name);

   if (prg_list_count>=COUNT_PRG) {
     uc1601("Число Операций-превышает допустимое (>300)\n Уменьшите количество выбора",1);
   	 return( FALSE );
   }
   prg_list[prg_list_count].num=prg_list_count;
   prg_list[prg_list_count].tag=tag;
   prg_list[prg_list_count].name[0]='\0';
   sprintf(prg_list[prg_list_count].name,"%s",name);
   prg_list_count++;

   return( TRUE );
}


/*****************************************************************************/
int _main_loadDll( void )
{
    int  response   = 0;
    int errorCode;
    char *dialog_name=actions->styler_file ;
    char env_names[][25]={
    "UGII_USER_DIR" ,
    "UGII_SITE_DIR" ,
    "UGII_VENDOR_DIR" ,
    "USER_UFUN" ,
    "UGII_INITIAL_UFUN_DIR" ,
    "UGII_TMP_DIR" ,
    "HOME" ,
    "TMP" } ;
 int i ;
 char *path , envpath[133] , dlgpath[255] , info[133];
 int status ;

 path = (char *) malloc(133+10);

 errorCode=-1;
  for (i=0;i<7;i++) {

    envpath[0]='\0';
    path=envpath;
    UF_translate_variable(env_names[i], &path);
    if (path!=NULL) {

       /*1*/
       dlgpath[0]='\0';
       strcpy(dlgpath,path); strcat(dlgpath,"\\application\\"); strcat(dlgpath,dialog_name);
       UF_print_syslog(dlgpath,FALSE);

       // работа с файлом
       UF_CFI_ask_file_exist (dlgpath, &status );
       if (!status) { errorCode=0; break ; }

       // работа с файлом
       UF_CFI_ask_file_exist (dlgpath, &status );
       if (!status) { errorCode=0; break ; }

       /*2*/
       dlgpath[0]='\0';
       strcpy(dlgpath,path); strcat(dlgpath,"\\"); strcat(dlgpath,dialog_name);
       UF_print_syslog(dlgpath,FALSE);

     } else { //if (envpath!=NULL)
      info[0]='\0'; sprintf (info,"Переменная %s - не установлена \n ",env_names[i]);
      UF_print_syslog(info,FALSE);
     }
  } // for

 if (errorCode!=0) {
    info[0]='\0'; sprintf (info,"Don't load %s  \n ",dialog_name);
    uc1601 (info, TRUE );
  } else {
       if ( ( errorCode = UF_STYLER_create_dialog ( dlgpath,
           SP_cbs,      /* Callbacks from dialog */
           SP_CB_COUNT, /* number of callbacks*/
           NULL,        /* This is your client data */
           &response ) ) != 0 )
        {
              /* Get the user function fail message based on the fail code.*/
              PrintErrorMessage( errorCode );
         }
  }

 return(errorCode);
}


/*****************************************************************************/
int cam5( void )
{
   int errorCode;

   int module_id;
   UF_ask_application_module(&module_id);
   if (UF_APP_CAM!=module_id) {
       uc1601("Запуск DLL - производится из модуля обработки\n - 2005г.",1);
       return (-1);
    }

    if (NULL_TAG==UF_PART_ask_display_part()) {
      uc1601("Cam-часть не активна.....\n программа прервана.",1);
      return (0);
    }

 errorCode=_main_loadDll( );

 return(0);
}


int _construct_cb ( int dialog_id )
{
    int obj_count = 0;
    tag_t *tags = NULL ;

 // выбранные обьекты и их кол-во
    UF_UI_ONT_ask_selected_nodes(&obj_count, &tags) ;
    if (obj_count<=0) { return(0); }

    cam5_ask( tags[0] ) ;

    UF_free(tags);

/********************************/
    UF_STYLER_item_value_type_t data  ;
    int irc ;

    data.item_attr=UF_STYLER_VALUE;

    data.item_id=SP_REAL_SURFACESPEED ;        data.value.real=sp.surface_speed;                  irc=UF_STYLER_set_value(dialog_id,&data);
    data.item_id=SP_REAL_FEEDPERTOOTH ;        data.value.real=sp.feed_per_tooth;                 irc=UF_STYLER_set_value(dialog_id,&data);
    data.item_id=SP_TOGGLE_SPINDLESPEED ;      data.value.integer=sp.tog_spindle_rpm;             irc=UF_STYLER_set_value(dialog_id,&data);
    data.item_id=SP_REAL_SPINDLESPEED ;        data.value.real=sp.spindle_rpm;                    irc=UF_STYLER_set_value(dialog_id,&data);
    data.item_id=SP_OPTION_SPINDLEOUTPUTMODE ; data.value.integer=sp.spindle_mode;                irc=UF_STYLER_set_value(dialog_id,&data);
    data.item_id=SP_WIDE_RANGE ;               data.value.string=sp.spindle_range;                irc=UF_STYLER_set_value(dialog_id,&data);
    data.item_id=SP_TOGGLE_RANGESTATUS ;       data.value.integer=sp.tog_spindle_range;           irc=UF_STYLER_set_value(dialog_id,&data);
    data.item_id=SP_WIDE_TEXT ;                data.value.string=sp.spindle_text;                 irc=UF_STYLER_set_value(dialog_id,&data);
    data.item_id=SP_TOGGLE_TEXTSTATUS ;        data.value.integer=sp.tog_spindle_text;            irc=UF_STYLER_set_value(dialog_id,&data);
    data.item_id=SP_OPTION_SPINDLEDIRECTION ;  data.value.integer=sp.spindle_direction_control;   irc=UF_STYLER_set_value(dialog_id,&data);

    UF_STYLER_free_value (&data) ;
/********************************/

  _range_status_cb ( dialog_id ) ;
  _text_status_cb ( dialog_id ) ;

	return (0);
}


int _radio_calc_cb ( int dialog_id )
{
  tag_t  tls = null_tag ;
  int    tool_flutes_number = 0 ;
  double tool_diameter = 0 ;
  UF_PARAM_feedrate_t  feed;
  double smm = 0, mm=0 ;
  double surface_speed , feed_per_tooth ;

  tag_t   prg = NULL_TAG;
  int obj_count = 0;
  tag_t *tags = NULL ;
  int calc ;
  UF_STYLER_item_value_type_t data  ;
  int irc ;

 // выбранные обьекты и их кол-во
 irc = UF_UI_ONT_ask_selected_nodes(&obj_count, &tags) ;
 if (obj_count<=0) { return(0); }

  data.item_attr=UF_STYLER_VALUE;

  data.item_id=SP_RADIO_CALC ;               irc=UF_STYLER_ask_value(dialog_id,&data); calc=data.value.integer;
  data.item_id=SP_REAL_SPINDLESPEED ;        irc=UF_STYLER_ask_value(dialog_id,&data); sp.spindle_rpm=data.value.real;

  if (calc==0) {
    prg = tags[0]; // идентификатор объекта
    UF_CALL( UF_OPER_ask_cutter_group(prg,&tls) );
  	if (tls!=null_tag) {
      UF_CALL( UF_PARAM_ask_double_value(tls,UF_PARAM_TL_DIAMETER,&tool_diameter) );
      UF_CALL( UF_PARAM_ask_int_value(tls,UF_PARAM_TL_NUM_FLUTES,&tool_flutes_number) );
      UF_CALL( UF_PARAM_ask_subobj_ptr_value (prg,UF_PARAM_FEED_CUT,&feed) );

      if (feed.value<=0.0) feed.value=0.0;
      smm = ( tool_diameter * 3.1415926 * sp.spindle_rpm ) / 1000.0 ;
      if (tool_flutes_number>0 && sp.spindle_rpm>0)
         mm = feed.value / ( tool_flutes_number * sp.spindle_rpm ) ;
      surface_speed = smm ;
      feed_per_tooth = mm ;

      data.item_id=SP_REAL_SURFACESPEED ; data.value.real=surface_speed;  irc=UF_STYLER_set_value(dialog_id,&data);
      data.item_id=SP_REAL_FEEDPERTOOTH ; data.value.real=feed_per_tooth; irc=UF_STYLER_set_value(dialog_id,&data);
    }
  }

  UF_free(tags);

  UF_STYLER_free_value (&data) ;

	return (0);
}

int _apply_cb ( int dialog_id )
{
    char str[133];
    int ecode ;
    tag_t   prg = NULL_TAG;
    int i , j , count = 0 ;
    int obj_count = 0;
    tag_t *tags = NULL ;
    int calc ;

/********************************/
    UF_STYLER_item_value_type_t data  ;
    int irc ;
    data.item_attr=UF_STYLER_VALUE;

    data.item_id=SP_RADIO_CALC ;               irc=UF_STYLER_ask_value(dialog_id,&data); calc=data.value.integer;

    data.item_id=SP_REAL_SURFACESPEED ;        irc=UF_STYLER_ask_value(dialog_id,&data); sp.surface_speed=data.value.real;
    data.item_id=SP_REAL_FEEDPERTOOTH ;        irc=UF_STYLER_ask_value(dialog_id,&data); sp.feed_per_tooth=data.value.real;
    data.item_id=SP_TOGGLE_SPINDLESPEED ;      irc=UF_STYLER_ask_value(dialog_id,&data); sp.tog_spindle_rpm=data.value.integer;
    data.item_id=SP_REAL_SPINDLESPEED ;        irc=UF_STYLER_ask_value(dialog_id,&data); sp.spindle_rpm=data.value.real;
    data.item_id=SP_OPTION_SPINDLEOUTPUTMODE ; irc=UF_STYLER_ask_value(dialog_id,&data); sp.spindle_mode=data.value.integer;
    data.item_id=SP_TOGGLE_RANGESTATUS ;       irc=UF_STYLER_ask_value(dialog_id,&data); sp.tog_spindle_range=data.value.integer;
    data.item_id=SP_WIDE_RANGE ;               irc=UF_STYLER_ask_value(dialog_id,&data); sprintf(sp.spindle_range,"%s",data.value.string);
    data.item_id=SP_TOGGLE_TEXTSTATUS ;        irc=UF_STYLER_ask_value(dialog_id,&data); sp.tog_spindle_text=data.value.integer;
    data.item_id=SP_WIDE_TEXT ;                irc=UF_STYLER_ask_value(dialog_id,&data); sprintf(sp.spindle_text,"%s",data.value.string);
    data.item_id=SP_OPTION_SPINDLEDIRECTION ;  irc=UF_STYLER_ask_value(dialog_id,&data); sp.spindle_direction_control=data.value.integer;

    UF_STYLER_free_value (&data) ;
/********************************/

 // выбранные обьекты и их кол-во
 ecode = UF_UI_ONT_ask_selected_nodes(&obj_count, &tags) ;
 if (obj_count<=0) {
    uc1601("Не выбрано операций или программ!\n ....",1);
    return(0);
 }

 UF_UI_toggle_stoplight(1);

 for(i=0,count=0;i<obj_count;i++)
 {
   prg = tags[i]; // идентификатор объекта

   prg_list_count=0;// заполняем структуру
   ecode = UF_NCGROUP_cycle_members( prg, cycleGeneratePrg,NULL ) ;

   if (prg_list_count==0) {
   	  count+=_run( prg , calc );
   } else for (j=0;j<prg_list_count;j++) {
             count+=_run( prg_list[j].tag , calc);
           }
 }

 UF_free(tags);

 //UF_DISP_refresh ();
 UF_UI_ONT_refresh();

 UF_UI_toggle_stoplight(0);

 str[0]='\0'; sprintf(str,"Изменено операций=%d \n ....",count);
 uc1601(str,1);

 return (0);
}


int _tog_spindle_speed_cb ( int dialog_id )
{
/********************************/
    int tog ;
    UF_STYLER_item_value_type_t data  ;
    int irc ;
    data.item_attr=UF_STYLER_VALUE;

    data.item_id=SP_TOGGLE_SPINDLESPEED ;      irc=UF_STYLER_ask_value(dialog_id,&data); tog=data.value.integer;

    if (tog==0) {
     data.item_id=SP_REAL_SPINDLESPEED ; data.value.real=0.0; irc=UF_STYLER_set_value(dialog_id,&data);
     data.item_id=SP_REAL_SURFACESPEED ; data.value.real=0.0; irc=UF_STYLER_set_value(dialog_id,&data);
     data.item_id=SP_REAL_FEEDPERTOOTH ; data.value.real=0.0; irc=UF_STYLER_set_value(dialog_id,&data);
    }

    UF_STYLER_free_value (&data) ;
/********************************/
	return (0);
}

int _spindle_speed_cb ( int dialog_id )
{
/********************************/
    double val ;
    UF_STYLER_item_value_type_t data  ;
    int irc ;
    data.item_attr=UF_STYLER_VALUE;

    data.item_id=SP_REAL_SPINDLESPEED ;  irc=UF_STYLER_ask_value(dialog_id,&data);  val=data.value.real;

    if (val==0) {
      data.item_id=SP_TOGGLE_SPINDLESPEED ; data.value.integer=0; irc=UF_STYLER_set_value(dialog_id,&data);
      data.item_id=SP_REAL_SURFACESPEED ; data.value.real=0.0; irc=UF_STYLER_set_value(dialog_id,&data);
      data.item_id=SP_REAL_FEEDPERTOOTH ; data.value.real=0.0; irc=UF_STYLER_set_value(dialog_id,&data);
    } else {
    	data.item_id=SP_TOGGLE_SPINDLESPEED ; data.value.integer=1; irc=UF_STYLER_set_value(dialog_id,&data);
    }

    UF_STYLER_free_value (&data) ;
/********************************/
	return (0);
}


int _range_status_cb ( int dialog_id )
{
/********************************/
    int tog ;
    UF_STYLER_item_value_type_t data  ;
    int irc ;
    data.item_attr=UF_STYLER_VALUE;

    data.item_id=SP_TOGGLE_RANGESTATUS ;       irc=UF_STYLER_ask_value(dialog_id,&data); tog=data.value.integer;

    data.item_attr=UF_STYLER_SENSITIVITY;
    data.item_id=SP_WIDE_RANGE ;
    data.value.integer=tog;
    irc=UF_STYLER_set_value(dialog_id,&data);

    UF_STYLER_free_value (&data) ;
/********************************/
	return (0);
}

int _text_status_cb ( int dialog_id )
{
/********************************/
    int tog ;
    UF_STYLER_item_value_type_t data  ;
    int irc ;
    data.item_attr=UF_STYLER_VALUE;

    data.item_id=SP_TOGGLE_TEXTSTATUS ;       irc=UF_STYLER_ask_value(dialog_id,&data); tog=data.value.integer;

    data.item_attr=UF_STYLER_SENSITIVITY;
    data.item_id=SP_WIDE_TEXT ;
    data.value.integer=tog;
    irc=UF_STYLER_set_value(dialog_id,&data);

    UF_STYLER_free_value (&data) ;
/********************************/
	return (0);
}


int _action_calc_act_cb ( int dialog_id )
{
/********************************/
  double n , F , smm , mm , D;
  int    z ;

  double  da[4];
  char menu[4][16] ;
  int  ia4[4];
  double ra5[4];
  char ca6[4 ][ 31 ]={"","","",""};
  int  ip7[4];
  int  i , response ;

  UF_STYLER_item_value_type_t data  ;
  int irc ;
  data.item_attr=UF_STYLER_VALUE;

  data.item_id=SP_REAL_SPINDLESPEED ;
  irc=UF_STYLER_ask_value(dialog_id,&data);
  n=data.value.real;

  // инициализация массива
  for(i=0;i<4;i++) { da[i]=0.;     ia4[i]=0;    ra5[i]=-1.;    ip7[i]=0;  }
  da[0]=n;

  /********************************************************************************/
  strcpy(&menu[0][0], "n(rev/min)=\0");   ia4[0]=(int)da[0];   ra5[0]=da[0];   ip7[0]=200;
  strcpy(&menu[1][0], "D(mm)=\0");        ia4[1]=(int)da[1];   ra5[1]=da[1];   ip7[1]=201;
  strcpy(&menu[2][0], "z (N teeths)=\0"); ia4[2]=(int)da[2];   ra5[2]=da[2];   ip7[2]=100;
  strcpy(&menu[3][0], "F(mm/min)=\0");    ia4[3]=(int)da[3];   ra5[3]=da[3];   ip7[3]=202;

  response = uc1613 ("..Параметры расчета режима резания..", menu, 4 ,ia4, ra5, ca6, ip7);
  if (response != 3 && response != 4) {
    UF_STYLER_free_value (&data) ;
  	return (-1);
  }

  for(i=0;i<4;i++) {
   if (ip7[i]>=100 && ip7[i]<=199) da[i]=ia4[i];
   if (ip7[i]>=200 && ip7[i]<=299) da[i]=ra5[i];
   if (ip7[i]>=300 && ip7[i]<=399) { ; }
  }

  for(i=0;i<4;i++) { if (da[i]<=0.) da[i]=0.; }

  n=da[0];
  D=da[1];
  z=(int)da[2];
  F=da[3];

  smm = 0 ; mm =0 ;
  smm = ( D * 3.1415926 * n ) / 1000.0 ;
  if (z>0 && n>0.)  mm = F / ( z * n ) ;

  data.item_id=SP_REAL_SURFACESPEED ; data.value.real=smm; irc=UF_STYLER_set_value(dialog_id,&data);
  data.item_id=SP_REAL_FEEDPERTOOTH ; data.value.real=mm ; irc=UF_STYLER_set_value(dialog_id,&data);
  data.item_id=SP_REAL_SPINDLESPEED ; data.value.real=n  ; irc=UF_STYLER_set_value(dialog_id,&data);
  _spindle_speed_cb ( dialog_id ) ;

  UF_STYLER_free_value (&data) ;
/********************************/
	return (0);
}


//----------------------------------------------------------------------------
//  Activation Methods
//----------------------------------------------------------------------------

//  Explicit Activation
//      This entry point is used to activate the application explicitly, as in
//      "File->Execute UG/Open->User Function..."
extern "C" DllExport void ufusr( char *parm, int *returnCode, int rlen )
{
    /* Initialize the API environment */
    int errorCode = UF_initialize();

    if ( 0 == errorCode )
    {
        /* TODO: Add your application code here */
        cam5();

        /* Terminate the API environment */
        errorCode = UF_terminate();
    }

    /* Print out any error messages */
    PrintErrorMessage( errorCode );
    *returnCode=0;
}

//----------------------------------------------------------------------------
//  Utilities
//----------------------------------------------------------------------------

// Unload Handler
//     This function specifies when to unload your application from Unigraphics.
//     If your application registers a callback (from a MenuScript item or a
//     User Defined Object for example), this function MUST return
//     "UF_UNLOAD_UG_TERMINATE".
extern "C" int ufusr_ask_unload (void)
{
     /* unload immediately after application exits*/
     return ( UF_UNLOAD_IMMEDIATELY );

     /*via the unload selection dialog... */
     /*return ( UF_UNLOAD_SEL_DIALOG );   */
     /*when UG terminates...              */
     /*return ( UF_UNLOAD_UG_TERMINATE ); */
}

/*--------------------------------------------------------------------------
You have the option of coding the cleanup routine to perform any housekeeping
chores that may need to be performed.  If you code the cleanup routine, it is
automatically called by Unigraphics.
--------------------------------------------------------------------------*/
extern void ufusr_cleanup (void)
{
    return;
}


/* PrintErrorMessage
**
**     Prints error messages to standard error and the Unigraphics status
**     line. */
static void PrintErrorMessage( int errorCode )
{
    if ( 0 != errorCode )
    {
        /* Retrieve the associated error message */
        char message[133];
        UF_get_fail_message( errorCode, message );

        /* Print out the message */
        UF_UI_set_status( message );

        fprintf( stderr, "%s\n", message );
    }
}




/*-------------------------------------------------------------------------*/
/*---------------------- UIStyler Callback Functions ----------------------*/
/*-------------------------------------------------------------------------*/

/* -------------------------------------------------------------------------
 * Callback Name: SP_construct_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int SP_construct_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _construct_cb ( dialog_id ) ;

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);
    /* A return value of UF_UI_CB_EXIT_DIALOG will not be accepted    */
    /* for this callback type.  You must continue dialog construction.*/

}


/* -------------------------------------------------------------------------
 * Callback Name: SP_apply_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int SP_apply_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _apply_cb ( dialog_id ) ;

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog                 */
    /* A return value of UF_UI_CB_EXIT_DIALOG will not be accepted    */
    /* for this callback type.  You must respond to your apply button.*/
    return (UF_UI_CB_CONTINUE_DIALOG);

}


/* -------------------------------------------------------------------------
 * Callback Name: SP_action_calc_act_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int SP_action_calc_act_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _action_calc_act_cb ( dialog_id ) ;

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.    */
    /* return ( UF_UI_CB_EXIT_DIALOG );               */

}


/* -------------------------------------------------------------------------
 * Callback Name: SP_radio_calc_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int SP_radio_calc_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _radio_calc_cb ( dialog_id );

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.  */
    /* return ( UF_UI_CB_EXIT_DIALOG );             */

}


/* -------------------------------------------------------------------------
 * Callback Name: SP_surface_speed_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int SP_surface_speed_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.    */
    /* return ( UF_UI_CB_EXIT_DIALOG );               */

}


/* -------------------------------------------------------------------------
 * Callback Name: SP_feed_per_tooth_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int SP_feed_per_tooth_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.    */
    /* return ( UF_UI_CB_EXIT_DIALOG );               */

}


/* -------------------------------------------------------------------------
 * Callback Name: SP_tog_spindle_speed_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int SP_tog_spindle_speed_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _tog_spindle_speed_cb ( dialog_id ) ;

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.  */
    /* return ( UF_UI_CB_EXIT_DIALOG );             */

}


/* -------------------------------------------------------------------------
 * Callback Name: SP_spindle_speed_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int SP_spindle_speed_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _spindle_speed_cb ( dialog_id );

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.    */
    /* return ( UF_UI_CB_EXIT_DIALOG );               */

}


/* -------------------------------------------------------------------------
 * Callback Name: SP_spindle_output_mode_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int SP_spindle_output_mode_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.    */
    /* return ( UF_UI_CB_EXIT_DIALOG );               */

}


/* -------------------------------------------------------------------------
 * Callback Name: SP_range_status_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int SP_range_status_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _range_status_cb ( dialog_id ) ;

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.  */
    /* return ( UF_UI_CB_EXIT_DIALOG );             */

}


/* -------------------------------------------------------------------------
 * Callback Name: SP_range_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int SP_range_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.    */
    /* return ( UF_UI_CB_EXIT_DIALOG );               */

}


/* -------------------------------------------------------------------------
 * Callback Name: SP_text_status_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int SP_text_status_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */
     _text_status_cb ( dialog_id ) ;

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.  */
    /* return ( UF_UI_CB_EXIT_DIALOG );             */

}


/* -------------------------------------------------------------------------
 * Callback Name: SP_text_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int SP_text_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.    */
    /* return ( UF_UI_CB_EXIT_DIALOG );               */

}


/* -------------------------------------------------------------------------
 * Callback Name: SP_spindle_direction_cb
 * This is a callback function associated with an action taken from a
 * UIStyler object.
 *
 * Input: dialog_id   -   The dialog id indicate which dialog this callback
 *                        is associated with.  The dialog id is a dynamic,
 *                        unique id and should not be stored.  It is
 *                        strictly for the use in the NX Open API:
 *                               UF_STYLER_ask_value(s)
 *                               UF_STYLER_set_value
 *        client_data -   Client data is user defined data associated
 *                        with your dialog.  Client data may be bound
 *                        to your dialog with UF_MB_add_styler_actions
 *                        or UF_STYLER_create_dialog.
 *        callback_data - This structure pointer contains information
 *                        specific to the UIStyler Object type that
 *                        invoked this callback and the callback type.
 * -----------------------------------------------------------------------*/
int SP_spindle_direction_cb ( int dialog_id,
             void * client_data,
             UF_STYLER_item_value_type_p_t callback_data)
{
     /* Make sure User Function is available. */
     if ( UF_initialize() != 0)
          return ( UF_UI_CB_CONTINUE_DIALOG );

     /* ---- Enter your callback code here ----- */

     UF_terminate ();

    /* Callback acknowledged, do not terminate dialog */
    return (UF_UI_CB_CONTINUE_DIALOG);

    /* or Callback acknowledged, terminate dialog.    */
    /* return ( UF_UI_CB_EXIT_DIALOG );               */

}






