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

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "xfpm-dpms-spins.h"
#include "xfpm-marshal.h"

#ifdef HAVE_DPMS

/* init */
static void xfpm_dpms_spins_init      (XfpmDpmsSpins *dpms_spins);
static void xfpm_dpms_spins_class_init(XfpmDpmsSpinsClass *klass);
static void xfpm_dpms_spins_finalize  (GObject *object);

static void xfpm_dpms_spins_get_spin1_value_cb(GtkSpinButton *spin_1,
                                               XfpmDpmsSpins *spins);
static void xfpm_dpms_spins_get_spin2_value_cb(GtkSpinButton *spin_2,
                                               XfpmDpmsSpins *spins);
static void xfpm_dpms_spins_get_spin3_value_cb(GtkSpinButton *spin_3,
                                               XfpmDpmsSpins *spins);

#define XFPM_DPMS_SPINS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o),XFPM_TYPE_DPMS_SPINS,XfpmDpmsSpinsPrivate))

struct XfpmDpmsSpinsPrivate 
{
    GtkWidget *spin_1;
    GtkWidget *spin_2;
    GtkWidget *spin_3;
    
};

G_DEFINE_TYPE(XfpmDpmsSpins,xfpm_dpms_spins,GTK_TYPE_TABLE);

enum {
    
    DPMS_VALUE_CHANGED,
    LAST_SIGNAL
    
};

static guint signals[LAST_SIGNAL] = { 0,}; 

static void
xfpm_dpms_spins_class_init(XfpmDpmsSpinsClass *klass) 
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    
    signals[DPMS_VALUE_CHANGED] = g_signal_new("dpms-value-changed",
					      					  XFPM_TYPE_DPMS_SPINS,
    									      G_SIGNAL_RUN_LAST,
    									      G_STRUCT_OFFSET(XfpmDpmsSpinsClass,dpms_value_changed),
    									      NULL,NULL,
    									      _xfpm_marshal_VOID__INT_INT_INT,
    									      G_TYPE_NONE,3,
    									      G_TYPE_INT,
    									      G_TYPE_INT,
    									      G_TYPE_INT);
    
    object_class->finalize = xfpm_dpms_spins_finalize;
    
    g_type_class_add_private(klass,sizeof(XfpmDpmsSpinsClass));
    
}

static void
xfpm_dpms_spins_init(XfpmDpmsSpins *dpms_spins)
{
    XfpmDpmsSpinsPrivate *priv;
    priv = XFPM_DPMS_SPINS_GET_PRIVATE(dpms_spins);
    
    g_object_set(G_OBJECT(dpms_spins),
                 "homogeneous",TRUE,
                 "n-columns",2,
                 "n-rows",3,
                 NULL);
                 
    GtkWidget *label;
    
    label = gtk_label_new(_("Standby after"));
    gtk_widget_show(label);
    gtk_table_attach_defaults(GTK_TABLE(dpms_spins),label,0,1,0,1);
    priv->spin_1 = gtk_spin_button_new_with_range(1,238,1);
    gtk_widget_show(priv->spin_1);
    gtk_table_attach(GTK_TABLE(dpms_spins),priv->spin_1,1,2,0,1,GTK_SHRINK,GTK_SHRINK,0,0);
    
    label = gtk_label_new(_("Suspend after"));
    gtk_widget_show(label);
    gtk_table_attach_defaults(GTK_TABLE(dpms_spins),label,0,1,1,2);
    priv->spin_2 = gtk_spin_button_new_with_range(1,239,1);
    gtk_widget_show(priv->spin_2);
    gtk_table_attach(GTK_TABLE(dpms_spins),priv->spin_2,1,2,1,2,GTK_SHRINK,GTK_SHRINK,0,0);
    
    label = gtk_label_new(_("Turn off after"));
    gtk_widget_show(label);
    gtk_table_attach_defaults(GTK_TABLE(dpms_spins),label,0,1,2,3);
    priv->spin_3 = gtk_spin_button_new_with_range(1,300,1);
    gtk_widget_show(priv->spin_3);
    gtk_table_attach(GTK_TABLE(dpms_spins),priv->spin_3,1,2,2,3,GTK_SHRINK,GTK_SHRINK,0,0);
    
    gtk_widget_show(GTK_WIDGET(dpms_spins));
    
    g_signal_connect(priv->spin_1,"value-changed",G_CALLBACK(xfpm_dpms_spins_get_spin1_value_cb),dpms_spins);
    g_signal_connect(priv->spin_2,"value-changed",G_CALLBACK(xfpm_dpms_spins_get_spin2_value_cb),dpms_spins);
    g_signal_connect(priv->spin_3,"value-changed",G_CALLBACK(xfpm_dpms_spins_get_spin3_value_cb),dpms_spins);
}

static void
xfpm_dpms_spins_finalize(GObject *object)
{
    XfpmDpmsSpins *spins;
    spins = XFPM_DPMS_SPINS(object);
    
    G_OBJECT_CLASS(xfpm_dpms_spins_parent_class)->finalize(object);
}

static void
xfpm_dpms_spins_get_spin1_value_cb(GtkSpinButton *spin_1,XfpmDpmsSpins *spins)
{
    XfpmDpmsSpinsPrivate *priv;
    priv = XFPM_DPMS_SPINS_GET_PRIVATE(spins);
    
    gint value1,value2,value3;
    value1 = gtk_spin_button_get_value(spin_1);
    
    value2 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(priv->spin_2));
    
    value3 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(priv->spin_3));
       
    if ( value2 <= value1 )
    {
        value2 = value1 + 1;
        /* gtk_spin_button_set_value generate a value-change signal, to avoid receiving
         * 3 callbacks we block then handler by func and then unblock it again */
        g_signal_handlers_block_by_func(priv->spin_2,xfpm_dpms_spins_get_spin2_value_cb,spins);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->spin_2),value2);
        g_signal_handlers_unblock_by_func(priv->spin_2,xfpm_dpms_spins_get_spin2_value_cb,spins);
    }
    
    if ( value3 <= value2 )
    {
        value3 = value2 + 1;
        g_signal_handlers_block_by_func(priv->spin_3,xfpm_dpms_spins_get_spin3_value_cb,spins);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->spin_3),value3);
        g_signal_handlers_unblock_by_func(priv->spin_3,xfpm_dpms_spins_get_spin3_value_cb,spins);
    }
    
    g_signal_emit(G_OBJECT(spins),signals[DPMS_VALUE_CHANGED],0,
                  value1,value2,value3);
    
}

static void
xfpm_dpms_spins_get_spin2_value_cb(GtkSpinButton *spin_2,XfpmDpmsSpins *spins)
{
    XfpmDpmsSpinsPrivate *priv;
    priv = XFPM_DPMS_SPINS_GET_PRIVATE(spins);
    
    gint value1,value2,value3;
    value2 = gtk_spin_button_get_value(spin_2);
    
    value1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(priv->spin_1));
    
    value3 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(priv->spin_3));

    if ( value2 <= value1 )
    {
        value2 = value1 + 1;
        g_signal_handlers_block_by_func(priv->spin_2,xfpm_dpms_spins_get_spin2_value_cb,spins);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->spin_2),value2);
        g_signal_handlers_unblock_by_func(priv->spin_2,xfpm_dpms_spins_get_spin2_value_cb,spins);
    } 
        
    if ( value3 <= value2 )
    {
        value3 = value2 + 1;
        g_signal_handlers_block_by_func(priv->spin_3,xfpm_dpms_spins_get_spin3_value_cb,spins);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->spin_3),value3);
        g_signal_handlers_unblock_by_func(priv->spin_3,xfpm_dpms_spins_get_spin3_value_cb,spins);
    }
    
    g_signal_emit(G_OBJECT(spins),signals[DPMS_VALUE_CHANGED],0,value1,value2,value3);  
    
}

static void
xfpm_dpms_spins_get_spin3_value_cb(GtkSpinButton *spin_3,XfpmDpmsSpins *spins)
{
    XfpmDpmsSpinsPrivate *priv;
    priv = XFPM_DPMS_SPINS_GET_PRIVATE(spins);
    
    gint value1,value2,value3;
    value3 = gtk_spin_button_get_value(spin_3);
    
    value1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(priv->spin_1));
    
    value2 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(priv->spin_2));

    if ( value3 <= value2 )
    {
        value3 = value2 + 1;
        g_signal_handlers_block_by_func(priv->spin_3,xfpm_dpms_spins_get_spin3_value_cb,spins);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->spin_3),value3);
        g_signal_handlers_unblock_by_func(priv->spin_3,xfpm_dpms_spins_get_spin3_value_cb,spins);
    }
    
    g_signal_emit(G_OBJECT(spins),signals[DPMS_VALUE_CHANGED],0,value1,value2,value3);
}

GtkWidget *
xfpm_dpms_spins_new(void)
{
    return GTK_WIDGET(g_object_new(XFPM_TYPE_DPMS_SPINS,NULL));

}

void  xfpm_dpms_spins_set_default_values(XfpmDpmsSpins *spins,
                                        guint spin_1,
                                        guint spin_2,
                                        guint spin_3)
{
    XfpmDpmsSpinsPrivate *priv;
    priv = XFPM_DPMS_SPINS_GET_PRIVATE(spins);
    
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->spin_1),spin_1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->spin_2),spin_2);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(priv->spin_3),spin_3);
    
}   

void   
xfpm_dpms_spins_set_active(XfpmDpmsSpins *spins,
                          gboolean active)
{
    XfpmDpmsSpinsPrivate *priv;
    priv = XFPM_DPMS_SPINS_GET_PRIVATE(spins);
    
    gtk_widget_set_sensitive(GTK_WIDGET(priv->spin_1),active);
    gtk_widget_set_sensitive(GTK_WIDGET(priv->spin_2),active);
    gtk_widget_set_sensitive(GTK_WIDGET(priv->spin_3),active);
    
}                     
                                        
#endif /* HAVE_DPMS */