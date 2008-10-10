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

#ifndef __XFPM_NOTIFY_H
#define __XFPM_NOTIFY_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#include <gtk/gtk.h>

void xfpm_notify_simple(const gchar *title,
			  	        const gchar *message,
						guint  timeout,
						NotifyUrgency urgency,
						GtkStatusIcon *icon,
						const gchar *icon_name,
						guint8 timeout_to_show);

void xfpm_notify_with_action(const gchar *title,
			   			     const gchar *message,
							 guint timeout,
							 NotifyUrgency urgency,
							 GtkStatusIcon *icon,
							 const gchar *icon_name,
							 const gchar *action_label,
							 guint8 timeout_to_show,
							 NotifyActionCallback notify_callback,
							 gpointer user_data);


#endif /* HAVE_LIBNOTIFY */

#endif /*__XFPM_NOTIFY_H */