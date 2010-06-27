/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 2; tab-width: 2 -*- */
/*
** Login : <ctaf42@gmail.com>
** Started on  Fri Nov 30 05:31:31 2007 GESTES Cedric
** $Id$
**
** Copyright (C) 2007 GESTES Cedric
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <stdlib.h>

#include "systray-config.h"
#include "systray-menu-functions.h"
#include "systray-notifications.h"
#include "na-tray.h"
#include "na-tray-manager.h"
#include "systray-struct.h"
#include "systray-init.h"


CD_APPLET_DEFINITION ("systray",
	1, 5, 4,
	CAIRO_DOCK_CATEGORY_DESKTOP,
	N_("Add a systray to your dock.\n"
	"Left-click to show the systray in a dialog (you can bind a keyboard shortcut for it.)\n"
	"Middle-click to close the dalog.\n"
	"But the best way to use it id to detach it from the dock, and place it somewhere, above other windows."),
	"Ctaf (Cedric Gestes)")


CD_APPLET_INIT_BEGIN
{
	CD_APPLET_REGISTER_FOR_CLICK_EVENT;
	CD_APPLET_REGISTER_FOR_MIDDLE_CLICK_EVENT;
	CD_APPLET_REGISTER_FOR_BUILD_MENU_EVENT;
	
	if (na_tray_manager_check_running (gtk_widget_get_screen (GTK_WIDGET (myContainer->pWidget))) && ! cairo_dock_is_loading ())
	{
		cairo_dock_show_temporary_dialog_with_icon (D_("Another systray is already running (probably on your panel)\nSince there can only be one systray at once, you should remove it to avoid any conflict."), myIcon, myContainer, 3000, NULL);
	}
	
	if (myDesklet != NULL)  // on cree le systray pour avoir qqch a afficher dans le desklet.
	{
		systray_build_and_show ();
		CD_APPLET_SET_STATIC_DESKLET;
	}
	if (myDock)  // en mode desklet, on n'a pas besoin de l'icone.
	{
		CD_APPLET_SET_DEFAULT_IMAGE_ON_MY_ICON_IF_NONE;
	}
}
CD_APPLET_INIT_END


CD_APPLET_STOP_BEGIN
{
  CD_APPLET_UNREGISTER_FOR_CLICK_EVENT;
  CD_APPLET_UNREGISTER_FOR_MIDDLE_CLICK_EVENT;
  CD_APPLET_UNREGISTER_FOR_BUILD_MENU_EVENT;
}
CD_APPLET_STOP_END


CD_APPLET_RELOAD_BEGIN
{
	if (CD_APPLET_MY_CONFIG_CHANGED)
	{
		if (myData.tray)
		{
			GtkOrientation o = na_tray_get_orientation (myData.tray);
			if ((o == GTK_ORIENTATION_HORIZONTAL && myConfig.iIconPacking == 1)  ||
				(o == GTK_ORIENTATION_VERTICAL && myConfig.iIconPacking == 0))
			{
				na_tray_set_orientation (myData.tray,
					myConfig.iIconPacking == 0 ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
			}
		}
		
		if (! myData.tray)
		{
			if (myDesklet != NULL)  // on cree le systray pour avoir qqch a afficher dans le desklet.
				systray_build_and_show ();
		}
		else if (CD_APPLET_MY_CONTAINER_TYPE_CHANGED)
		{
			if (myDesklet != NULL)  // il faut passer du dialogue au desklet.
			{
				cairo_dock_steal_interactive_widget_from_dialog (myData.dialog);
				cairo_dock_dialog_unreference (myData.dialog);
				myData.dialog = NULL;
				cairo_dock_add_interactive_widget_to_desklet (GTK_WIDGET (myData.tray), myDesklet);
				cairo_dock_set_desklet_renderer_by_name (myDesklet, NULL, ! CAIRO_DOCK_LOAD_ICONS_FOR_DESKLET, NULL);
				CD_APPLET_SET_STATIC_DESKLET;
			}
			else  // il faut passer du desklet au dialogue
			{
				CairoDesklet *pDesklet = CAIRO_DESKLET (CD_APPLET_MY_OLD_CONTAINER);
				cairo_dock_steal_interactive_widget_from_desklet (pDesklet);
				myData.dialog = cd_systray_build_dialog ();
				//myData.dialog = cairo_dock_build_dialog (NULL, myIcon, myContainer, NULL, myData.tray->widget, GTK_BUTTONS_NONE, NULL, NULL, NULL);
				cairo_dock_hide_dialog (myData.dialog);
			}
		}

		if (myData.tray)
		{
			systray_apply_settings();
		}
		
		if (myDock)  // en mode desklet, on n'a pas besoin de l'icone.
		{
			CD_APPLET_SET_DEFAULT_IMAGE_ON_MY_ICON_IF_NONE;
		}
	}
}
CD_APPLET_RELOAD_END
