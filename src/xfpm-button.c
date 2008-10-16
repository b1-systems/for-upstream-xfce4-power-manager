/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*-
 *
 * * Copyright (C) 2008 Ali <ali.slackware@gmail.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <xfconf/xfconf.h>

#include "xfpm-button.h"
#include "xfpm-hal.h"
#include "xfpm-debug.h"
#include "xfpm-common.h"
#include "xfpm-notify.h"
#include "xfpm-enums.h"
#include "xfpm-enum-types.h"
#include "xfpm-marshal.h"

#define XFPM_BUTTON_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE(o,XFPM_TYPE_BUTTON,XfpmButtonPrivate))

static void xfpm_button_init(XfpmButton *bt);
static void xfpm_button_class_init(XfpmButtonClass *klass);
static void xfpm_button_finalize(GObject *object);

static void xfpm_button_set_property (GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec);
static void xfpm_button_get_property (GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec);

static void xfpm_button_do_suspend(XfpmButton *bt);
static void xfpm_button_do_hibernate(XfpmButton *bt);

static void xfpm_button_lid_pressed(XfpmButton *bt,const gchar *udi);
static void xfpm_button_power_pressed(XfpmButton *bt,const gchar *udi);
static void xfpm_button_sleep_pressed(XfpmButton *bt,const gchar *udi);

static void xfpm_button_handle_device_condition_cb(XfpmHal *hal,
                                                   const gchar *udi,
                                                   const gchar *condition_name,
                                                   const gchar *condition_detail,
                                                   XfpmButton *bt);

static void xfpm_button_load_config(XfpmButton *bt);
static void xfpm_button_get_switches(XfpmButton *bt);

struct XfpmButtonPrivate
{
    XfpmHal *hal;
    gulong handler_id;
    
    gboolean have_lid_bt;
    gboolean lid_button_has_state;
    
    gboolean have_power_bt;
    gboolean power_button_has_state;
    
    gboolean have_sleep_bt;
    gboolean sleep_button_has_state;
    
    gboolean device_found;
};

G_DEFINE_TYPE(XfpmButton,xfpm_button,G_TYPE_OBJECT)

enum 
{
    XFPM_ACTION_REQUEST,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0,}; 

enum
{
    PROP_0,
    PROP_LID_ACTION,
    PROP_SLEEP_ACTION,
    PROP_POWER_ACTION
};

static void xfpm_button_class_init(XfpmButtonClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = xfpm_button_finalize;
    
    gobject_class->set_property = xfpm_button_set_property;
    gobject_class->get_property = xfpm_button_get_property;

    signals[XFPM_ACTION_REQUEST] = g_signal_new("xfpm-action-request",
                                               XFPM_TYPE_BUTTON,
                                               G_SIGNAL_RUN_LAST,
                                               G_STRUCT_OFFSET(XfpmButtonClass,button_action_request),
                                               NULL,NULL,
                                               _xfpm_marshal_VOID__ENUM_BOOLEAN ,
                                               G_TYPE_NONE,2,
                                               XFPM_TYPE_ACTION_REQUEST,G_TYPE_BOOLEAN);
                                                   
    g_object_class_install_property(gobject_class,
                                    PROP_LID_ACTION,
                                    g_param_spec_enum("lid-switch-action",
                                                      "lid switch action",
                                                      "lid switch action",
                                                      XFPM_TYPE_ACTION_REQUEST,
                                                      XFPM_DO_NOTHING,
                                                      G_PARAM_READWRITE));    
    
    g_object_class_install_property(gobject_class,
                                    PROP_SLEEP_ACTION,
                                    g_param_spec_enum("sleep-switch-action",
                                                      "sleep switch action",
                                                      "sleep switch action",
                                                      XFPM_TYPE_ACTION_REQUEST,
                                                      XFPM_DO_NOTHING,
                                                      G_PARAM_READWRITE));    
    g_object_class_install_property(gobject_class,
                                    PROP_POWER_ACTION,
                                    g_param_spec_enum("power-switch-action",
                                                      "power switch action",
                                                      "power switch action",
                                                      XFPM_TYPE_ACTION_REQUEST,
                                                      XFPM_DO_NOTHING,
                                                      G_PARAM_READWRITE));                                                      
    
    g_type_class_add_private(klass,sizeof(XfpmButtonPrivate));

}

static void xfpm_button_init(XfpmButton *bt)
{
    XfpmButtonPrivate *priv;
    priv = XFPM_BUTTON_GET_PRIVATE(bt);
    
    priv->have_lid_bt           = FALSE;
    priv->lid_button_has_state  = FALSE;
    
    priv->have_power_bt         = FALSE;
    priv->power_button_has_state= FALSE;
    
    priv->have_sleep_bt         = FALSE;
    priv->sleep_button_has_state= FALSE;
    
    priv->hal = xfpm_hal_new();
    
    xfpm_button_load_config(bt);
    xfpm_button_get_switches(bt);
    
    if ( priv->have_lid_bt || priv->have_power_bt || priv->have_sleep_bt )
    {
        if (xfpm_hal_connect_to_signals(priv->hal,FALSE,FALSE,FALSE,TRUE) )
        {
            priv->handler_id =
            g_signal_connect(priv->hal,"xfpm-device-condition",
                            G_CALLBACK(xfpm_button_handle_device_condition_cb),bt);
        }
    }
}

static void xfpm_button_set_property (GObject *object,
                                      guint prop_id,
                                      const GValue *value,
                                      GParamSpec *pspec)
{
#ifdef DEBUG
    gchar *content;
    content = g_strdup_value_contents(value);
    XFPM_DEBUG("param:%s value contents:%s\n",pspec->name,content);
    g_free(content);
#endif      
    XfpmButton *bt;
    bt = XFPM_BUTTON(object);
    
    switch (prop_id)
    {
    case PROP_LID_ACTION:
        bt->lid_action = g_value_get_enum(value);
        break;   
    case PROP_SLEEP_ACTION:
        bt->sleep_action = g_value_get_enum(value);
        break;   
     case PROP_POWER_ACTION:
        bt->power_action = g_value_get_enum(value);
        break;           
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
        break;
    }
}
                                           
static void xfpm_button_get_property (GObject *object,
                                      guint prop_id,
                                      GValue *value,
                                      GParamSpec *pspec)
{
    XfpmButton *bt;
    bt = XFPM_BUTTON(object);
        
    switch (prop_id)
    {
    case  PROP_LID_ACTION:
        g_value_set_enum(value,bt->lid_action);
        break;   
     case PROP_SLEEP_ACTION:
        g_value_set_enum(value,bt->sleep_action);
        break;   
     case PROP_POWER_ACTION:
        g_value_set_enum(value,bt->power_action);
        break;       
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
        break;
    }                         
    
#ifdef DEBUG
    gchar *content;
    content = g_strdup_value_contents(value);
    XFPM_DEBUG("param:%s value contents:%s\n",pspec->name,content);
    g_free(content);
#endif  
    
}

static void xfpm_button_finalize(GObject *object)
{
    XfpmButton *bt;
    bt = XFPM_BUTTON(object);
    
    bt->priv = XFPM_BUTTON_GET_PRIVATE(bt);
    
    if ( bt->priv->hal ) 
    {
        g_object_unref(bt->priv->hal);
    }
    
    G_OBJECT_CLASS(xfpm_button_parent_class)->finalize(object);
}

static gboolean
_unblock_handler(XfpmButtonPrivate *priv)
{
    g_signal_handler_unblock(priv->hal,priv->handler_id);
    return FALSE;
}

static void
xfpm_button_do_hibernate(XfpmButton *bt)
{
    XfpmButtonPrivate *priv;
    priv = XFPM_BUTTON_GET_PRIVATE(bt);
    
    g_signal_handler_block(priv->hal,priv->handler_id);
    
    g_signal_emit(G_OBJECT(bt),signals[XFPM_ACTION_REQUEST],0,XFPM_DO_HIBERNATE,FALSE);
    
    g_timeout_add(10,(GSourceFunc)_unblock_handler,priv);
    
}

static void
xfpm_button_do_suspend(XfpmButton *bt)
{
    XfpmButtonPrivate *priv;
    priv = XFPM_BUTTON_GET_PRIVATE(bt);
    
    g_signal_handler_block(priv->hal,priv->handler_id);
    
    g_signal_emit(G_OBJECT(bt),signals[XFPM_ACTION_REQUEST],0,XFPM_DO_SUSPEND,FALSE);
    
    g_timeout_add(10,(GSourceFunc)_unblock_handler,priv);
}

static void
_proccess_action(XfpmButton *bt,XfpmActionRequest action)
{
    if ( action == XFPM_DO_SUSPEND )
    {
         xfpm_button_do_suspend(bt);
    }
    else if ( action == XFPM_DO_HIBERNATE )
    {
         xfpm_button_do_hibernate(bt);
    }
    else if ( action == XFPM_DO_SHUTDOWN )
    {
        XfpmButtonPrivate *priv;
        priv = XFPM_BUTTON_GET_PRIVATE(bt);
        g_signal_handler_block(priv->hal,priv->handler_id);
    
        g_signal_emit(G_OBJECT(bt),signals[XFPM_ACTION_REQUEST],0,XFPM_DO_SHUTDOWN,FALSE);
        
        g_timeout_add(10,(GSourceFunc)_unblock_handler,priv);
        
    }
        
}

static void
xfpm_button_lid_pressed(XfpmButton *bt,const gchar *udi)
{
    if ( bt->lid_action == XFPM_DO_NOTHING )
    {
        return;
    }
    
    XfpmButtonPrivate *priv;
    priv = XFPM_BUTTON_GET_PRIVATE(bt);
    
    if ( priv->have_lid_bt )
    {
        if ( priv->lid_button_has_state )
        {
            GError *error = NULL;
            gboolean pressed = 
            xfpm_hal_get_bool_info(priv->hal,udi,"button.state.value",&error);
            if ( error ) 
            {
                XFPM_DEBUG("Error getting lid switch state: %s\n",error->message);
                g_error_free(error);
                return;
            }
            else if ( pressed == TRUE )
            {
                _proccess_action(bt,bt->lid_action);
            }
        }
        else 
        {
            _proccess_action(bt,bt->lid_action);
        }
    }
}

static void
xfpm_button_power_pressed(XfpmButton *bt,const gchar *udi)
{
    if ( bt->power_action == XFPM_DO_NOTHING )
    {
        XFPM_DEBUG("Power action is do nothing\n");
        return;
    }
    
    XfpmButtonPrivate *priv;
    priv = XFPM_BUTTON_GET_PRIVATE(bt);
    
    if ( priv->have_power_bt )
    {
        if ( priv->power_button_has_state )
        {
            GError *error = NULL;
            gboolean pressed = 
            xfpm_hal_get_bool_info(priv->hal,udi,"button.state.value",&error);
            if ( error ) 
            {
                XFPM_DEBUG("Error getting lid switch state: %s\n",error->message);
                g_error_free(error);
                return;
            }
            else if ( pressed == TRUE )
            {
                _proccess_action(bt,bt->power_action);
            }
        }
        else 
        {
            _proccess_action(bt,bt->power_action);
        }
    }
}

static void
xfpm_button_sleep_pressed(XfpmButton *bt,const gchar *udi)
{
    XfpmButtonPrivate *priv;
    priv = XFPM_BUTTON_GET_PRIVATE(bt);
    
    if ( bt->sleep_action == XFPM_DO_NOTHING )
    {
        return;
    }
    
    if ( priv->have_sleep_bt )
    {
        if ( priv->sleep_button_has_state )
        {
            GError *error = NULL;
            gboolean pressed = 
            xfpm_hal_get_bool_info(priv->hal,udi,"button.state.value",&error);
            if ( error ) 
            {
                XFPM_DEBUG("Error getting lid switch state: %s\n",error->message);
                g_error_free(error);
                return;
            }
            else if ( pressed == TRUE )
            {
                _proccess_action(bt,bt->sleep_action);
            }
        }
        else 
        {
            _proccess_action(bt,bt->sleep_action);
        }
    }
}

static void
xfpm_button_handle_device_condition_cb(XfpmHal *hal,
                                       const gchar *udi,
                                       const gchar *condition_name,
                                       const gchar *condition_detail,
                                       XfpmButton *bt)
{
    XfpmButtonPrivate *priv;
    priv = XFPM_BUTTON_GET_PRIVATE(bt);
    
    if ( xfpm_hal_device_have_capability(priv->hal,udi,"button") )
    {
        if ( strcmp(condition_name,"ButtonPressed") )
        {
            XFPM_DEBUG("Not processing event with condition_name=%s\n",condition_name);
            return;
        }
        XFPM_DEBUG("proccessing event: %s %s\n",condition_name,condition_detail);
        if ( !strcmp(condition_detail,"lid") )
        {
            xfpm_button_lid_pressed(bt,udi);
        }
        else if ( !strcmp(condition_detail,"power") )
        {
            xfpm_button_power_pressed(bt,udi);
        }
        else if ( !strcmp(condition_detail,"sleep") )
        {
            xfpm_button_sleep_pressed(bt,udi);
        }
    }
}                                       

static void 
xfpm_button_load_config(XfpmButton *bt)
{
    XFPM_DEBUG("Loading configuration\n");
    
    GError *g_error = NULL;
    if ( !xfconf_init(&g_error) )
    {
        g_critical("xfconf init failed: %s\n",g_error->message);
        XFPM_DEBUG("Using default values\n");
        g_error_free(g_error);
        bt->lid_action = XFPM_DO_NOTHING;
        bt->sleep_action = XFPM_DO_NOTHING;
        bt->power_action = XFPM_DO_NOTHING;
        return;
    }
    XfconfChannel *channel;
    
    channel = xfconf_channel_new(XFPM_CHANNEL_CFG);

    bt->lid_action   = xfconf_channel_get_uint(channel,LID_SWITCH_CFG,XFPM_DO_NOTHING);
    bt->sleep_action   = xfconf_channel_get_uint(channel,SLEEP_SWITCH_CFG,XFPM_DO_NOTHING);
    bt->power_action = xfconf_channel_get_uint(channel,POWER_SWITCH_CFG,XFPM_DO_NOTHING);
    
    g_object_unref(channel);
    xfconf_shutdown();    
}

static void
_check_button(XfpmButtonPrivate *priv,const gchar *button_type,const gchar *udi)
{
    if ( !strcmp(button_type,"lid"))
    {
        priv->have_lid_bt = TRUE;
        priv->lid_button_has_state = xfpm_hal_get_bool_info(priv->hal,udi,
                                                            "button.has_state",
                                                            NULL);
                                                            
    }
    else if ( !strcmp(button_type,"power") )
    {
        priv->have_power_bt = TRUE;
        priv->power_button_has_state = xfpm_hal_get_bool_info(priv->hal,udi,
                                                            "button.has_state",
                                                            NULL);
    }
    else if ( !strcmp(button_type,"sleep"))
    {
        priv->have_sleep_bt = TRUE;
        priv->sleep_button_has_state = xfpm_hal_get_bool_info(priv->hal,udi,
                                                            "button.has_state",
                                                            NULL);
    }
}

static void
xfpm_button_get_switches(XfpmButton *bt)
{
    XfpmButtonPrivate *priv;
    priv = XFPM_BUTTON_GET_PRIVATE(bt);
    
    gchar **udi;
    gint dummy;
    GError *error = NULL;
    
    udi = xfpm_hal_get_device_udi_by_capability(priv->hal,"button",&dummy,&error);
    
    if ( error )
    {
        XFPM_DEBUG("Unable to get button switches: %s\n",error->message);
        g_error_free(error);
        return;
    }
    
    if ( !udi )
    {
        XFPM_DEBUG("No buttons found\n");
    }
    int i = 0 ;
    for ( i = 0 ; udi[i] ; i++)
    {
        if ( xfpm_hal_device_have_key(priv->hal,udi[i],"button.type") && 
            xfpm_hal_device_have_key(priv->hal,udi[i],"button.has_state") )
        {
            GError *error = NULL;
            gchar *button_type = 
            xfpm_hal_get_string_info(priv->hal,udi[i],"button.type",&error);
            if ( error )
            {
                XFPM_DEBUG(":%s\n",error->message);
                g_error_free(error);
                continue;
            }
            if ( button_type)
            {
                _check_button(priv,button_type,udi[i]);
                libhal_free_string(button_type);
            }
        }
    }
#ifdef DEBUG
    gchar *content1;
    gchar *content2;
    
    GValue value = { 0, };
    g_value_init(&value,G_TYPE_BOOLEAN);
    
    g_value_set_boolean(&value,priv->have_lid_bt);
    content1 = g_strdup_value_contents(&value);
    
    g_value_set_boolean(&value,priv->lid_button_has_state);    
    content2 = g_strdup_value_contents(&value);
    
    XFPM_DEBUG("LID Switch found  = %s has_state = %s\n",content1,content2);
    g_free(content1);
    g_free(content2);
    
    g_value_set_boolean(&value,priv->have_power_bt);
    content1 = g_strdup_value_contents(&value);
    
    g_value_set_boolean(&value,priv->power_button_has_state);    
    content2 = g_strdup_value_contents(&value);
    
    XFPM_DEBUG("Power Switch found = %s has_state = %s\n",content1,content2);
    g_free(content1);
    g_free(content2);
    
    g_value_set_boolean(&value,priv->have_sleep_bt);
    content1 = g_strdup_value_contents(&value);
    
    g_value_set_boolean(&value,priv->sleep_button_has_state);    
    content2 = g_strdup_value_contents(&value);
    
    XFPM_DEBUG("Sleep Switch found = %s has_state = %s\n",content1,content2);
    g_free(content1);
    g_free(content2);
#endif    
    libhal_free_string_array(udi);
}

XfpmButton *
xfpm_button_new(void)
{
    XfpmButton *bt;
    bt = g_object_new(XFPM_TYPE_BUTTON,NULL);
    return bt;
}

guint8
xfpm_button_get_available_buttons(XfpmButton *button)
{
    g_return_val_if_fail(XFPM_IS_BUTTON(button),0);
    
    XfpmButtonPrivate *priv;
    priv = XFPM_BUTTON_GET_PRIVATE(button);
    
    guint8 buttons = 0;
    
    if ( priv->have_lid_bt )
    {
        buttons |= LID_SWITCH;
    }
    if ( priv->have_power_bt )
    {
        buttons |= POWER_SWITCH;
    }
    if ( priv->have_sleep_bt )
    {
        buttons |= SLEEP_SWITCH;
    }
    
    return buttons;
}
