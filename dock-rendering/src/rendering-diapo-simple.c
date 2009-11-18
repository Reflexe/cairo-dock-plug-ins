/**
* This file is a part of the Cairo-Dock project
*
* Copyright : (C) see the 'copyright' file.
* E-mail    : see the 'copyright' file.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 3
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <cairo.h>

#include "rendering-diapo-simple.h"

extern gint     my_diapo_simple_iconGapX;
extern gint     my_diapo_simple_iconGapY;
extern gdouble  my_diapo_simple_fScaleMax;
extern gint     my_diapo_simple_sinW;
extern gboolean my_diapo_simple_lineaire;
extern gboolean  my_diapo_simple_wide_grid;

extern gdouble  my_diapo_simple_color_frame_start[4];
extern gdouble  my_diapo_simple_color_frame_stop[4];
extern gboolean my_diapo_simple_fade2bottom;
extern gboolean my_diapo_simple_fade2right;
extern guint    my_diapo_simple_arrowWidth;
extern guint    my_diapo_simple_arrowHeight;
extern gdouble  my_diapo_simple_arrowShift;
extern guint    my_diapo_simple_lineWidth;
extern guint    my_diapo_simple_radius;
extern gdouble  my_diapo_simple_color_border_line[4];
extern gboolean my_diapo_simple_draw_background;
extern gboolean my_diapo_simple_display_all_icons;

const  gint X_BORDER_SPACE = 40;
const  gint ARROW_TIP = 10;  // pour gerer la pointe de la fleche.

/// On considere qu'on a my_diapo_simple_iconGapX entre chaque icone horizontalement, et my_diapo_simple_iconGapX/2 entre les icones et les bords (pour aerer un peu plus le dessin). Idem verticalement. X_BORDER_SPACE est la pour empecher que les icones debordent de la fenetre au zoom.


// Fonctions utiles pour transformer l'index de la liste en couple (x,y) sur la grille et inversement.
static inline void _get_gridXY_from_index (guint nRowsX, guint index, guint* gridX, guint* gridY)
{
	*gridX = index % nRowsX;
	*gridY = index / nRowsX;
}
static inline guint _get_index_from_gridXY (guint nRowsX, guint gridX, guint gridY)
{
	return gridX + gridY * nRowsX;
}

static guint _cd_rendering_diapo_simple_guess_grid (GList *pIconList, guint *nRowX, guint *nRowY)
{
	// Calcul du nombre de lignes (nY) / colonnes (nX) :
	guint count = g_list_length (pIconList);
	if (count == 0)
	{
		*nRowX = 0;
		*nRowY = 0;
	}
	else if (my_diapo_simple_wide_grid)
	{
		*nRowX = ceil(sqrt(count));
		*nRowY = ceil(((double) count) / *nRowX);
	}
	else
	{
		*nRowY = ceil(sqrt(count));
		*nRowX = ceil(((double) count) / *nRowY);
	}
	return count;
}


void cd_rendering_calculate_max_dock_size_diapo_simple (CairoDock *pDock)
{
	// On calcule la configuration de la grille
	guint nRowsX = 0;  // nb colonnes.
	guint nRowsY = 0;  // nb lignes.
	guint nIcones = 0;  // nb icones.
	nIcones = _cd_rendering_diapo_simple_guess_grid(pDock->icons, &nRowsX, &nRowsY);
	
	// On calcule la taille de l'affichage
	if(nIcones != 0)
	{
		int iMaxIconWidth = ((Icon*)pDock->icons->data)->fWidth;  // approximation un peu bof.
		pDock->iMaxDockWidth = nRowsX * (iMaxIconWidth + my_diapo_simple_iconGapX) + 2*X_BORDER_SPACE;
		pDock->iMaxDockHeight = (nRowsY - 1) * (pDock->iMaxIconHeight + my_diapo_simple_iconGapY) +  // les icones
			pDock->iMaxIconHeight * my_diapo_simple_fScaleMax +  // les icones des bords zooment
			myLabels.iLabelSize +  // le texte des icones de la 1ere ligne
			my_diapo_simple_lineWidth + // les demi-lignes du haut et du bas
			my_diapo_simple_arrowHeight + ARROW_TIP;  // la fleche etendue
		pDock->iMinDockWidth = pDock->iMaxDockWidth - 2*X_BORDER_SPACE;
		pDock->iMinDockHeight = pDock->iMaxDockHeight;
	}
	else
	{
		pDock->iMaxDockWidth = pDock->iMinDockWidth = X_BORDER_SPACE * 2 + 1;
		pDock->iMaxDockHeight = pDock->iMinDockHeight = my_diapo_simple_lineWidth + my_diapo_simple_arrowHeight + ARROW_TIP + 1;
	}
	
	// pas de decorations.
	pDock->iDecorationsHeight = 0;
	pDock->iDecorationsWidth  = 0;
	
	// On affecte ca aussi au cas ou.
	pDock->fFlatDockWidth = pDock->iMinDockWidth - 2*X_BORDER_SPACE;
	pDock->fMagnitudeMax = my_diapo_simple_fScaleMax / (1+g_fAmplitude);
}


void cd_rendering_render_diapo_simple (cairo_t *pCairoContext, CairoDock *pDock)
{
	if (my_diapo_simple_draw_background)
	{
		//\____________________ On trace le cadre.
		cairo_save (pCairoContext);
		cairo_dock_draw_frame_for_diapo_simple (pCairoContext, pDock);
		
		//\____________________ On dessine les decorations dedans.
		double fAlpha = (pDock->fFoldingFactor < .3 ? (.3 - pDock->fFoldingFactor) / .3 : 0.);  // apparition du cadre de 0.3 a 0
		cairo_dock_render_decorations_in_frame_for_diapo_simple (pCairoContext, pDock, fAlpha);

		//\____________________ On dessine le cadre.
		if (my_diapo_simple_lineWidth != 0 && my_diapo_simple_color_border_line[3] != 0 && fAlpha != 0)
		{
			cairo_set_line_width (pCairoContext,  my_diapo_simple_lineWidth);
			cairo_set_source_rgba (pCairoContext,
				my_diapo_simple_color_border_line[0],
				my_diapo_simple_color_border_line[1],
				my_diapo_simple_color_border_line[2],
				my_diapo_simple_color_border_line[3] * fAlpha);
			cairo_stroke (pCairoContext);
		}
		else
			cairo_new_path (pCairoContext);
		cairo_restore (pCairoContext);
	}
	
	if (pDock->icons == NULL)
		return;
	
	//\____________________ On dessine la ficelle qui les joint.
	//TODO Rendre joli !
	if (myIcons.iStringLineWidth > 0)
		cairo_dock_draw_string (pCairoContext, pDock, myIcons.iStringLineWidth, TRUE, TRUE);
	
	//\____________________ On dessine les icones avec leurs etiquettes.
	// on determine la 1ere icone a tracer : l'icone suivant l'icone pointee.
	GList *pFirstDrawnElement = cairo_dock_get_first_drawn_element_linear (pDock->icons);
	if (pFirstDrawnElement == NULL)
		return;
	
	// on dessine les icones, l'icone pointee en dernier.
	Icon *icon;
	GList *ic = pFirstDrawnElement;
	do
	{
		icon = ic->data;
		
		if (icon->bPointed)
		{
			
		}
		
		cairo_save (pCairoContext);
		cairo_dock_render_one_icon (icon, pDock, pCairoContext, 1., FALSE);
		cairo_restore (pCairoContext);
		
//////////////////////////////////////////////////////////////////////////////////////// On affiche le texte !
		gdouble zoom;
		if(icon->pTextBuffer != NULL && (my_diapo_simple_display_all_icons || icon->bPointed))
		{
			double fAlpha = (pDock->fFoldingFactor > .5 ? (1 - pDock->fFoldingFactor) / .5 : 1.);
			cairo_save (pCairoContext);
			
			double fOffsetX = -icon->fTextXOffset + icon->fWidthFactor * icon->fWidth * icon->fScale / 2;
			if (fOffsetX < 0)
				fOffsetX = 0;
			else if (0 + fOffsetX + icon->iTextWidth > pDock->container.iWidth)
				fOffsetX = pDock->container.iWidth - icon->iTextWidth - 0;
			
			if (icon->iTextWidth > icon->fWidth * icon->fScale + my_diapo_simple_iconGapX && ! icon->bPointed)
			{
				if (pDock->container.bIsHorizontal)
				{
					cairo_translate (pCairoContext,
						icon->fDrawX - my_diapo_simple_iconGapX/2,
						icon->fDrawY - icon->iTextHeight);
				}
				else
				{
					cairo_translate (pCairoContext,
						icon->fDrawY - my_diapo_simple_iconGapX/2,
						icon->fDrawX - icon->iTextHeight);
				}
				cairo_set_source_surface (pCairoContext,
					icon->pTextBuffer,
					fOffsetX,
					0.);
				
				cairo_pattern_t *pGradationPattern = cairo_pattern_create_linear (0.,
					0.,
					icon->fWidth * icon->fScale + my_diapo_simple_iconGapX,
					0.);
				cairo_pattern_set_extend (pGradationPattern, icon->bPointed ? CAIRO_EXTEND_PAD : CAIRO_EXTEND_NONE);
				cairo_pattern_add_color_stop_rgba (pGradationPattern,
					0.,
					0.,
					0.,
					0.,
					fAlpha);
				cairo_pattern_add_color_stop_rgba (pGradationPattern,
					0.75,
					0.,
					0.,
					0.,
					fAlpha);
				cairo_pattern_add_color_stop_rgba (pGradationPattern,
					1.,
					0.,
					0.,
					0.,
					MIN (0.2, fAlpha/2));
				cairo_mask (pCairoContext, pGradationPattern);
				cairo_pattern_destroy (pGradationPattern);
			}
			else  // le texte tient dans l'icone.
			{
				if (pDock->container.bIsHorizontal)
				{
					fOffsetX = icon->fDrawX + (icon->fWidth * icon->fScale - icon->iTextWidth) / 2;
					if (fOffsetX < 0)
						fOffsetX = 0;
					else if (fOffsetX + icon->iTextWidth > pDock->container.iWidth)
						fOffsetX = pDock->container.iWidth - icon->iTextWidth;
					cairo_translate (pCairoContext,
						fOffsetX,
						icon->fDrawY - icon->iTextHeight);
				}
				else
				{
					fOffsetX = icon->fDrawY + (icon->fWidth * icon->fScale - icon->iTextWidth) / 2;
					if (fOffsetX < 0)
						fOffsetX = 0;
					else if (fOffsetX + icon->iTextWidth > pDock->container.iHeight)
						fOffsetX = pDock->container.iHeight - icon->iTextWidth;
					cairo_translate (pCairoContext,
						fOffsetX,
						icon->fDrawX - icon->iTextHeight);
				}
				cairo_set_source_surface (pCairoContext,
					icon->pTextBuffer,
					0.,
					0.);
				cairo_paint_with_alpha (pCairoContext, fAlpha);
			}
			cairo_restore (pCairoContext);
		}
		
		ic = cairo_dock_get_next_element (ic, pDock->icons);
	}
	while (ic != pFirstDrawnElement);
}



static Icon* _cd_rendering_calculate_icons_for_diapo_simple (CairoDock *pDock, guint nRowsX, guint nRowsY, gint Mx, gint My)
{
	// On calcule la position de base pour toutes les icones
	int iOffsetY = .5 * pDock->iMaxIconHeight * (my_diapo_simple_fScaleMax - 1) +  // les icones de la 1ere ligne zooment
		myLabels.iLabelSize +  // le texte des icones de la 1ere ligne
		.5 * my_diapo_simple_lineWidth;  // demi-ligne du haut;
	
	double fFoldingX = (pDock->fFoldingFactor > .2 ? (pDock->fFoldingFactor - .2) / .8 : 0.);  // placement de 1 a 0.2
	double fFoldingY = (pDock->fFoldingFactor > .5 ? (pDock->fFoldingFactor - .5) / .5 : 0.);  // placement de 1 a 0.5
	Icon* icon;
	GList* ic, *pointed_ic=NULL;
	int i, x, y;
	for (ic = pDock->icons, i = 0; ic != NULL; ic = ic->next, i++)
	{
		icon = ic->data;
		
		// position sur la grille.
		_get_gridXY_from_index(nRowsX, i, &x, &y);
		
		// on en deduit la position au repos.
		icon->fX = X_BORDER_SPACE + .5*my_diapo_simple_iconGapX + (icon->fWidth + my_diapo_simple_iconGapX) * x;
		icon->fY = iOffsetY + (icon->fHeight + my_diapo_simple_iconGapY) * y;
		if (!pDock->container.bDirectionUp)
			icon->fY = pDock->container.iHeight - icon->fY - icon->fHeight;
		
		// on en deduit le zoom par rapport a la position de la souris.
		gdouble distanceE = sqrt ((icon->fX + icon->fWidth/2 - Mx) * (icon->fX + icon->fWidth/2 - Mx) + (icon->fY + icon->fHeight/2 - My) * (icon->fY + icon->fHeight/2 - My));
		if (my_diapo_simple_lineaire)
		{
			gdouble eloignementMax = my_diapo_simple_sinW;  // 3. * (icon->fWidth + icon->fHeight)  / 2
			icon->fScale = MAX (1., my_diapo_simple_fScaleMax + (1. - my_diapo_simple_fScaleMax) * distanceE / eloignementMax);
			icon->fPhase = 0.;
		}
		else
		{
			icon->fPhase = distanceE * G_PI / my_diapo_simple_sinW + G_PI / 2.;
			if (icon->fPhase < 0)
				icon->fPhase = 0;
			else if (icon->fPhase > G_PI)
				icon->fPhase = G_PI;
			icon->fScale = 1. + (my_diapo_simple_fScaleMax - 1.) * sin (icon->fPhase);
		}
		
		// on tient compte du zoom (zoom centre).
		icon->fXMin = icon->fXMax = icon->fXAtRest =  // Ca on s'en sert pas encore
		icon->fDrawX = icon->fX + icon->fWidth  * (1. - icon->fScale) / 2;
		icon->fDrawY = icon->fY + icon->fHeight * (1. - icon->fScale) / 2;
		
		// on tient compte du depliage.
		icon->fDrawX -= (icon->fDrawX - pDock->container.iWidth/2) * fFoldingX;
		icon->fDrawY = icon->fDrawY + (pDock->container.bDirectionUp ?
			pDock->container.iHeight - (my_diapo_simple_arrowHeight + ARROW_TIP + icon->fDrawY) :
			- icon->fDrawY) * fFoldingY;
		
		icon->fAlpha = (pDock->fFoldingFactor > .7 ? (1 - pDock->fFoldingFactor) / (1 - .7) : 1.);  // apparition de 1 a 0.7
		
		// On regarde si l'icone est pointee, si oui elle est un peu plus visible que les autres.
		if((Mx > icon->fX - .5*my_diapo_simple_iconGapX) && 
			(My > icon->fY - .5*my_diapo_simple_iconGapY) &&
			(Mx < icon->fX + icon->fWidth  + .5*my_diapo_simple_iconGapX) &&
			(My < icon->fY + icon->fHeight + .5*my_diapo_simple_iconGapY))
		{
			icon->bPointed = TRUE;
			pointed_ic = ic;
		}
		else
		{
			icon->bPointed = FALSE;
			///icon->fAlpha *= 0.75;
		}
		
		// On affecte tous les parametres qui n'ont pas été défini précédement
		icon->fPhase = 0.;
		icon->fOrientation = 0 * 2. * G_PI * pDock->fFoldingFactor;
		icon->fWidthFactor = icon->fHeightFactor = (pDock->fFoldingFactor > .7 ? (1 - pDock->fFoldingFactor) / .3 : 1.);
		//icon->fWidthFactor = icon->fHeightFactor = 1.;
	}
	return (pointed_ic == NULL ? NULL : pointed_ic->data);
}
static void _cd_rendering_check_if_mouse_inside_diapo_simple (CairoDock *pDock)
{
	if ((pDock->container.iMouseX < 0) || (pDock->container.iMouseX > pDock->iMaxDockWidth - 1) || (pDock->container.iMouseY < 0) || (pDock->container.iMouseY > pDock->iMaxDockHeight - 0))
	{
		pDock->iMousePositionType = CAIRO_DOCK_MOUSE_OUTSIDE;
	}
	else  // on fait simple.
	{
		pDock->iMousePositionType = CAIRO_DOCK_MOUSE_INSIDE;
	}
}
static void _cd_rendering_check_can_drop_linear (CairoDock *pDock)
{
	pDock->bCanDrop = TRUE;  /// caluler bCanDrop ...
}
Icon *cd_rendering_calculate_icons_diapo_simple (CairoDock *pDock)
{
	// On calcule la configuration de la grille
	guint nRowsX = 0;
	guint nRowsY = 0;
	guint nIcones = 0;
	nIcones = _cd_rendering_diapo_simple_guess_grid (pDock->icons, &nRowsX, &nRowsY);
	
	// On calcule les parametres des icones
	Icon *pPointedIcon = _cd_rendering_calculate_icons_for_diapo_simple (pDock, nRowsX, nRowsY, pDock->container.iMouseX, pDock->container.iMouseY);
	
	_cd_rendering_check_if_mouse_inside_diapo_simple (pDock);
	
	_cd_rendering_check_can_drop_linear (pDock);
	
	return pPointedIcon;
}


void cd_rendering_register_diapo_simple_renderer (const gchar *cRendererName)
{
//////////////////////////////////////////////////////////////////////////////////////// On definit le renderer :
	CairoDockRenderer *pRenderer = g_new0 (CairoDockRenderer, 1);                                           //Nouvelle structure	
	pRenderer->cReadmeFilePath = g_strdup_printf ("%s/readme-diapo-simple-view", MY_APPLET_SHARE_DATA_DIR);        //On affecte le readme
	pRenderer->cPreviewFilePath = g_strdup_printf ("%s/preview-diapo-simple.jpg", MY_APPLET_SHARE_DATA_DIR);       // la preview
	pRenderer->compute_size = cd_rendering_calculate_max_dock_size_diapo_simple;                        //La fonction qui défini les bornes     
	pRenderer->calculate_icons = cd_rendering_calculate_icons_diapo_simple;                                        //qui calcule les param des icones      
	pRenderer->render = cd_rendering_render_diapo_simple;                                                          //qui initie le calcul du rendu         
	pRenderer->render_optimized = NULL;//cd_rendering_render_diapo_simple_optimized;                                      //pareil en mieux                       
	pRenderer->set_subdock_position = cairo_dock_set_subdock_position_linear;                               // ?                                    
	pRenderer->render_opengl = cd_rendering_render_diapo_simple_opengl;
	
	pRenderer->bUseReflect = FALSE;                                                                         // On dit non au reflections
	pRenderer->cDisplayedName = D_ (cRendererName);
	
	cairo_dock_register_renderer (cRendererName, pRenderer);                                    //Puis on signale l'existence de notre rendu
}


//////////////////////////////////////////////////////////////////////////////////////// Methodes de dessin :

static void cairo_dock_draw_frame_horizontal_for_diapo_simple (cairo_t *pCairoContext, CairoDock *pDock)
{
	const gdouble arrow_dec = 2;
	gdouble fFrameWidth  = pDock->iMaxDockWidth - 2 * X_BORDER_SPACE;
	gdouble fFrameHeight = pDock->iMaxDockHeight - (my_diapo_simple_arrowHeight + ARROW_TIP + my_diapo_simple_lineWidth);
	gdouble fDockOffsetX = X_BORDER_SPACE;
	gdouble fDockOffsetY = (pDock->container.bDirectionUp ? .5*my_diapo_simple_lineWidth : my_diapo_simple_arrowHeight + ARROW_TIP);
	
	cairo_move_to (pCairoContext, fDockOffsetX, fDockOffsetY);

	//HautGauche -> HautDroit
	if(pDock->container.bDirectionUp)
	{
		cairo_rel_line_to (pCairoContext, fFrameWidth, 0);
	}
	else
	{
		//On fait la fleche
		cairo_rel_line_to (pCairoContext,  (fFrameWidth/2 - my_diapo_simple_arrowWidth/2 - my_diapo_simple_arrowShift * fFrameWidth), 0);                //     _
		cairo_rel_line_to (pCairoContext, + my_diapo_simple_arrowWidth/2 + my_diapo_simple_arrowShift * fFrameWidth - my_diapo_simple_arrowShift * fFrameWidth / arrow_dec,  -my_diapo_simple_arrowHeight);       //  \.
		cairo_rel_line_to (pCairoContext, + my_diapo_simple_arrowWidth/2 - my_diapo_simple_arrowShift * fFrameWidth + my_diapo_simple_arrowShift * fFrameWidth / arrow_dec, +my_diapo_simple_arrowHeight);        //    /
		cairo_rel_line_to (pCairoContext, (fFrameWidth/2 - my_diapo_simple_arrowWidth/2 + my_diapo_simple_arrowShift * fFrameWidth) , 0);               // _
	}
	//\_________________ Coin haut droit.
	cairo_rel_curve_to (pCairoContext,
		0, 0,
		my_diapo_simple_radius, 0,
		my_diapo_simple_radius, my_diapo_simple_radius );
	
	//HautDroit -> BasDroit
	cairo_rel_line_to (pCairoContext, 0, fFrameHeight + my_diapo_simple_lineWidth - my_diapo_simple_radius *  2 );
	//\_________________ Coin bas droit.
	cairo_rel_curve_to (pCairoContext,
		0, 0,
		0 , my_diapo_simple_radius,
		-my_diapo_simple_radius , my_diapo_simple_radius);
	
	//BasDroit -> BasGauche
	if(!pDock->container.bDirectionUp)
	{
		cairo_rel_line_to (pCairoContext, - fFrameWidth , 0);
	}
	else
	{
		//On fait la fleche
		cairo_rel_line_to (pCairoContext, - (fFrameWidth/2 - my_diapo_simple_arrowWidth/2 - my_diapo_simple_arrowShift * fFrameWidth), 0);                //     _
		cairo_rel_line_to (pCairoContext, - my_diapo_simple_arrowWidth/2 - my_diapo_simple_arrowShift * fFrameWidth + my_diapo_simple_arrowShift * fFrameWidth / arrow_dec, my_diapo_simple_arrowHeight);        //    /     
		cairo_rel_line_to (pCairoContext, - my_diapo_simple_arrowWidth/2 + my_diapo_simple_arrowShift * fFrameWidth - my_diapo_simple_arrowShift * fFrameWidth / arrow_dec, -my_diapo_simple_arrowHeight);       //  \. 
		cairo_rel_line_to (pCairoContext, - (fFrameWidth/2 - my_diapo_simple_arrowWidth/2 + my_diapo_simple_arrowShift * fFrameWidth) , 0);               // _      
	}
	//\_________________ Coin bas gauche.
	cairo_rel_curve_to (pCairoContext,
		0, 0,
		-my_diapo_simple_radius, 0,
		-my_diapo_simple_radius, -my_diapo_simple_radius);
	
	//BasGauche -> HautGauche
	cairo_rel_line_to (pCairoContext, 0, - fFrameHeight - my_diapo_simple_lineWidth + my_diapo_simple_radius * 2);
	//\_________________ Coin haut gauche.
	cairo_rel_curve_to (pCairoContext,
		0, 0,
		0 , -my_diapo_simple_radius ,
		my_diapo_simple_radius, -my_diapo_simple_radius);

}
static void cairo_dock_draw_frame_vertical_for_diapo_simple (cairo_t *pCairoContext, CairoDock *pDock)
{
	const gdouble arrow_dec = 2;
	gdouble fFrameWidth  = pDock->iMaxDockWidth - 2 * X_BORDER_SPACE;
	gdouble fFrameHeight = pDock->iMaxDockHeight - (my_diapo_simple_arrowHeight + ARROW_TIP + my_diapo_simple_lineWidth);
	gdouble fDockOffsetX = X_BORDER_SPACE;
	gdouble fDockOffsetY = (pDock->container.bDirectionUp ? .5*my_diapo_simple_lineWidth : my_diapo_simple_arrowHeight + ARROW_TIP);
	
	cairo_move_to (pCairoContext, fDockOffsetY, fDockOffsetX);

	if(pDock->container.bDirectionUp)
	{
		cairo_rel_line_to (pCairoContext, 0, fFrameWidth);
	}
	else
	{
		cairo_rel_line_to (pCairoContext,0,(fFrameWidth/2 - my_diapo_simple_arrowWidth/2 - my_diapo_simple_arrowShift * fFrameWidth));                //     _
		cairo_rel_line_to (pCairoContext, -my_diapo_simple_arrowHeight, my_diapo_simple_arrowWidth/2 + my_diapo_simple_arrowShift * fFrameWidth - my_diapo_simple_arrowShift * fFrameWidth / arrow_dec);       //  \. 
		cairo_rel_line_to (pCairoContext, my_diapo_simple_arrowHeight, + my_diapo_simple_arrowWidth/2 - my_diapo_simple_arrowShift * fFrameWidth + my_diapo_simple_arrowShift * fFrameWidth / arrow_dec);        //    /     
		cairo_rel_line_to (pCairoContext,0,(fFrameWidth/2 - my_diapo_simple_arrowWidth/2 + my_diapo_simple_arrowShift * fFrameWidth));               // _     
	}
	//\_________________ Coin haut droit.
	cairo_rel_curve_to (pCairoContext,
		0, 0,
		0, my_diapo_simple_radius,
		my_diapo_simple_radius, my_diapo_simple_radius);
	cairo_rel_line_to (pCairoContext, fFrameHeight + my_diapo_simple_lineWidth - my_diapo_simple_radius * 2, 0);
	//\_________________ Coin bas droit.
	cairo_rel_curve_to (pCairoContext,
			0, 0,
			my_diapo_simple_radius, 0,
			my_diapo_simple_radius, -my_diapo_simple_radius);
	if(!pDock->container.bDirectionUp)
	{
		cairo_rel_line_to (pCairoContext, 0, - fFrameWidth);
	}
	else
	{
		//On fait la fleche
		cairo_rel_line_to (pCairoContext, 0, - (fFrameWidth/2 - my_diapo_simple_arrowWidth/2 - my_diapo_simple_arrowShift * fFrameWidth));                 //     _
		cairo_rel_line_to (pCairoContext,  my_diapo_simple_arrowHeight, - my_diapo_simple_arrowWidth/2 - my_diapo_simple_arrowShift * fFrameWidth + my_diapo_simple_arrowShift * fFrameWidth / arrow_dec);        //    /     
		cairo_rel_line_to (pCairoContext, -my_diapo_simple_arrowHeight, - my_diapo_simple_arrowWidth/2 + my_diapo_simple_arrowShift * fFrameWidth - my_diapo_simple_arrowShift * fFrameWidth / arrow_dec );       //  \. 
		cairo_rel_line_to (pCairoContext, 0, - (fFrameWidth/2 - my_diapo_simple_arrowWidth/2 + my_diapo_simple_arrowShift * fFrameWidth));                 // _      
	}
	
	//\_________________ Coin bas gauche.
	cairo_rel_curve_to (pCairoContext,
		0, 0,
		0, -my_diapo_simple_radius,
		-my_diapo_simple_radius, -my_diapo_simple_radius);
	cairo_rel_line_to (pCairoContext, - fFrameHeight - my_diapo_simple_lineWidth + my_diapo_simple_radius * 2, 0);
	//\_________________ Coin haut gauche.
	cairo_rel_curve_to (pCairoContext,
		0, 0,
		-my_diapo_simple_radius, 0,
		-my_diapo_simple_radius, my_diapo_simple_radius);
}
void cairo_dock_draw_frame_for_diapo_simple (cairo_t *pCairoContext, CairoDock *pDock)
{
	if (pDock->container.bIsHorizontal)
		cairo_dock_draw_frame_horizontal_for_diapo_simple (pCairoContext, pDock);
	else
		cairo_dock_draw_frame_vertical_for_diapo_simple (pCairoContext, pDock);
}



void cairo_dock_render_decorations_in_frame_for_diapo_simple (cairo_t *pCairoContext, CairoDock *pDock, double fAlpha)
{
	// On se fait un beau pattern degrade :
	cairo_pattern_t *mon_super_pattern;
	mon_super_pattern = cairo_pattern_create_linear (0.0,
		0.0,
		my_diapo_simple_fade2right  ? pDock->iMaxDockWidth  : 0.0, // Y'aurait surement des calculs complexes à faire mais 
		my_diapo_simple_fade2bottom ? pDock->iMaxDockHeight : 0.0);     //  a quelques pixels près pour un dégradé : OSEF !
			
	cairo_pattern_add_color_stop_rgba (mon_super_pattern, 0, 
		my_diapo_simple_color_frame_start[0],
		my_diapo_simple_color_frame_start[1],
		my_diapo_simple_color_frame_start[2],
		my_diapo_simple_color_frame_start[3] * fAlpha);  // transparent -> opaque au depliage.
		
	cairo_pattern_add_color_stop_rgba (mon_super_pattern, 1, 
		my_diapo_simple_color_frame_stop[0],
		my_diapo_simple_color_frame_stop[1],
		my_diapo_simple_color_frame_stop[2],
		my_diapo_simple_color_frame_stop[3] * fAlpha);
	cairo_set_source (pCairoContext, mon_super_pattern);
	
	//On remplit le contexte en le préservant -> pourquoi ?  ----> parce qu'on va tracer le contour plus tard ;-)
	cairo_fill_preserve (pCairoContext);
	cairo_pattern_destroy (mon_super_pattern);
}


void cd_rendering_render_diapo_simple_opengl (CairoDock *pDock)
{
	//\____________________ On initialise le cadre.
	int iNbVertex;
	GLfloat *pColorTab, *pVertexTab;
	
	double fRadius = my_diapo_simple_radius;
	double fFrameWidth  = pDock->iMaxDockWidth - 2*X_BORDER_SPACE;  // longueur du trait horizontal.
	double fFrameHeight = pDock->iMaxDockHeight- (my_diapo_simple_arrowHeight + ARROW_TIP + my_diapo_simple_lineWidth);  // hauteur du cadre avec les rayons et sans la pointe.
	double fDockOffsetX, fDockOffsetY;
	if (pDock->container.bIsHorizontal)
	{
		fDockOffsetX = X_BORDER_SPACE;
		fDockOffsetY = (!pDock->container.bDirectionUp ? .5*my_diapo_simple_lineWidth : my_diapo_simple_arrowHeight+ARROW_TIP);
		fFrameWidth  = pDock->iMaxDockWidth - 2*X_BORDER_SPACE;  // longueur du trait horizontal.
		fFrameHeight = pDock->iMaxDockHeight- (my_diapo_simple_arrowHeight + ARROW_TIP + my_diapo_simple_lineWidth);  // hauteur du cadre avec les rayons et sans la pointe.
	}
	else
	{
		fDockOffsetY = X_BORDER_SPACE;
		fDockOffsetX = (pDock->container.bDirectionUp ? .5*my_diapo_simple_lineWidth : my_diapo_simple_arrowHeight+ARROW_TIP);
		fFrameHeight = pDock->iMaxDockWidth - 2*X_BORDER_SPACE;  // longueur du trait horizontal.
		fFrameWidth = pDock->iMaxDockHeight- (my_diapo_simple_arrowHeight + ARROW_TIP + my_diapo_simple_lineWidth);  // hauteur du cadre avec les rayons et sans la pointe.
	}
	
	
	if (my_diapo_simple_draw_background)  // On remplit le cadre en 2 temps (avec des polygones convexes).
	{
		glPushMatrix ();
		glTranslatef ((int) (fDockOffsetX + fFrameWidth/2), (int) (fDockOffsetY + fFrameHeight/2), -100);  // (int) -pDock->iMaxIconHeight * (1 + myIcons.fAmplitude) + 1
		glScalef (fFrameWidth, fFrameHeight, 1.);
		
		glEnable (GL_BLEND); // On active le blend
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_cairo_dock_set_blend_alpha ();
		
		double fAlpha = (pDock->fFoldingFactor < .3 ? (.3 - pDock->fFoldingFactor) / .3 : 0.);  // apparition du cadre de 0.3 a 0
		
		glPolygonMode (GL_FRONT, GL_FILL);
		glEnableClientState (GL_VERTEX_ARRAY);
		glEnableClientState (GL_COLOR_ARRAY);
		
		//\____________________ On remplit le cadre sans la fleche.
		pVertexTab = cd_rendering_generate_path_for_diapo_simple_opengl_without_arrow (pDock, fAlpha, &pColorTab, &iNbVertex);
		
		//glVertexPointer (_CAIRO_DOCK_PATH_DIM, GL_FLOAT, 0, pVertexTab);
		_cairo_dock_set_vertex_pointer (pVertexTab);
		glColorPointer (4, GL_FLOAT, 0, pColorTab);
		glDrawArrays (GL_POLYGON, 0, iNbVertex);
		
		glDisableClientState (GL_COLOR_ARRAY);
		
		//\____________________ On remplit la fleche.
		GLfloat color[4];
		pVertexTab = cd_rendering_generate_arrow_path_for_diapo_simple_opengl (pDock, fAlpha, color);
		glColor4fv (color);
		
		//glVertexPointer (_CAIRO_DOCK_PATH_DIM, GL_FLOAT, 0, pVertexTab);
		_cairo_dock_set_vertex_pointer (pVertexTab);
		glDrawArrays (GL_POLYGON, 0, 4);
		
		glDisableClientState (GL_VERTEX_ARRAY);
		
		//\____________________ On genere et trace le contour du cadre complet (avec la fleche).
		if (my_diapo_simple_lineWidth != 0 && my_diapo_simple_color_border_line[3] != 0 && fAlpha != 0)
		{
			pVertexTab = cd_rendering_generate_path_for_diapo_simple_opengl (pDock, &iNbVertex);
			
			//glVertexPointer (_CAIRO_DOCK_PATH_DIM, GL_FLOAT, 0, pVertexTab);
			_cairo_dock_set_vertex_pointer (pVertexTab);
			double frame_alpha = my_diapo_simple_color_border_line[3];
			my_diapo_simple_color_border_line[3] *= fAlpha;
			cairo_dock_draw_current_path_opengl (my_diapo_simple_lineWidth, my_diapo_simple_color_border_line, iNbVertex);
			my_diapo_simple_color_border_line[3] = frame_alpha;
		}
		
		glPopMatrix ();
	}
	
	if (pDock->icons == NULL)
		return ;
	
	_cairo_dock_set_blend_over ();
		
	//\____________________ On dessine la ficelle.
	if (myIcons.iStringLineWidth > 0)
		cairo_dock_draw_string_opengl (pDock, myIcons.iStringLineWidth, FALSE, FALSE);
	
	//\____________________ On dessine les icones.
	// on determine la 1ere icone a tracer : l'icone suivant l'icone pointee.
	GList *pFirstDrawnElement = cairo_dock_get_first_drawn_element_linear (pDock->icons);
	if (pFirstDrawnElement == NULL)
		return;
	
	// on dessine les icones, l'icone pointee en dernier.
	Icon *icon;
	GList *ic = pFirstDrawnElement;
	do
	{
		icon = ic->data;
		
		glPushMatrix ();
		
		if (icon->bPointed)
		{
			double fX, fY;
			if (pDock->container.bIsHorizontal)
			{
				fY = pDock->container.iHeight - icon->fDrawY + icon->fHeight * (my_diapo_simple_fScaleMax - icon->fScale)/2;
				fX = icon->fDrawX - icon->fWidth * (my_diapo_simple_fScaleMax - icon->fScale)/2;
			}
			else
			{
				fX = icon->fDrawY - icon->fHeight * (my_diapo_simple_fScaleMax - icon->fScale)/2;
				fY =  pDock->container.iWidth - icon->fDrawX + icon->fWidth * (my_diapo_simple_fScaleMax - icon->fScale)/2;
			}
			/*double r = my_diapo_simple_radius/2;
			double fColor[4] = {MAX (my_diapo_simple_color_frame_start[0], my_diapo_simple_color_frame_stop[0]) + .1,
				MAX (my_diapo_simple_color_frame_start[1], my_diapo_simple_color_frame_stop[1]) + .1,
				MAX (my_diapo_simple_color_frame_start[2], my_diapo_simple_color_frame_stop[2]) + .1,
				MAX (my_diapo_simple_color_frame_start[3], my_diapo_simple_color_frame_stop[3]) + .1};
			cairo_dock_draw_rounded_rectangle_opengl (r,
				0,
				icon->fWidth * my_diapo_simple_fScaleMax - r,
				icon->fHeight * my_diapo_simple_fScaleMax,
				fX, fY,
				fColor);*/
		}
		
		cairo_dock_render_one_icon_opengl (icon, pDock, 1., FALSE);
		
		if(icon->iLabelTexture != 0 && (my_diapo_simple_display_all_icons || icon->bPointed))
		{
			double fAlpha = (pDock->fFoldingFactor > .5 ? (1 - pDock->fFoldingFactor) / .5 : 1.);  // apparition du texte de 1 a 0.5
			double fOffsetX = 0.;
			if (icon->fDrawX + icon->fWidth * icon->fScale/2 - icon->iTextWidth/2 < 0)
				fOffsetX = icon->iTextWidth/2 - (icon->fDrawX + icon->fWidth * icon->fScale/2);
			else if (icon->fDrawX + icon->fWidth * icon->fScale/2 + icon->iTextWidth/2 > pDock->container.iWidth)
				fOffsetX = pDock->container.iWidth - (icon->fDrawX + icon->fWidth * icon->fScale/2 + icon->iTextWidth/2);
			
			glTranslatef (fOffsetX,
				(pDock->container.bDirectionUp ? 1:-1) * (icon->fHeight * icon->fScale/2 + myLabels.iLabelSize - icon->iTextHeight / 2),
				0.);
			
			if (icon->iTextWidth > icon->fWidth * icon->fScale + my_diapo_simple_iconGapX && ! icon->bPointed)
			{
				_cairo_dock_enable_texture ();
				_cairo_dock_set_blend_alpha ();
				_cairo_dock_set_alpha (fAlpha * icon->fScale / my_diapo_simple_fScaleMax);
				glBindTexture (GL_TEXTURE_2D, icon->iLabelTexture);
				
				double w = icon->fWidth * icon->fScale + my_diapo_simple_iconGapX;
				double h = icon->iTextHeight;
				_cairo_dock_apply_current_texture_portion_at_size_with_offset (0., 0., w/icon->iTextWidth, 1.,
					w, h, 0., 0.);
				
				_cairo_dock_disable_texture ();
			}
			else
			{
				cairo_dock_draw_texture_with_alpha (icon->iLabelTexture,
					icon->iTextWidth,
					icon->iTextHeight,
					fAlpha * icon->fScale / my_diapo_simple_fScaleMax);
			}
		}
		
		glPopMatrix ();
		
		ic = cairo_dock_get_next_element (ic, pDock->icons);
	}
	while (ic != pFirstDrawnElement);
}


static const double a = 2.5;  // definit combien la fleche est penchee.

#define RADIAN (G_PI / 180.0)  // Conversion Radian/Degres
#define DELTA_ROUND_DEGREE 5
#define TIP_OFFSET_FACTOR 2.
#define _recopy_prev_color(pColorTab, i) memcpy (&pColorTab[4*i], &pColorTab[4*(i-1)], 4*sizeof (GLfloat));
#define _copy_color(pColorTab, i, fAlpha, c) do { \
	pColorTab[4*i]   = c[0];\
	pColorTab[4*i+1] = c[1];\
	pColorTab[4*i+2] = c[2];\
	pColorTab[4*i+3] = c[3] * fAlpha; } while (0)
/*#define _copy_mean_color(pColorTab, i, fAlpha, c1, c2, f) do { \
	pColorTab[4*i]   = c1[0]*f + c2[0]*(1-f);\
	pColorTab[4*i+1] = c1[1]*f + c2[1]*(1-f);\
	pColorTab[4*i+2] = c1[2]*f + c2[2]*(1-f);\
	pColorTab[4*i+3] = (c1[3]*f + c2[3]*(1-f)) * fAlpha; } while (0)*/
GLfloat *cd_rendering_generate_path_for_diapo_simple_opengl (CairoDock *pDock, int *iNbPoints)
{
	//static GLfloat pVertexTab[((90/DELTA_ROUND_DEGREE+1)*4+1+3)*3];  // +3 pour la pointe.
	_cairo_dock_define_static_vertex_tab ((90/DELTA_ROUND_DEGREE+1)*4+1+3);  // +3 pour la pointe.
	double fRadius = my_diapo_simple_radius;
	double fFrameWidth  = pDock->iMaxDockWidth - 2*X_BORDER_SPACE - 2*fRadius;  // longueur du trait horizontal.
	double fFrameHeight = pDock->iMaxDockHeight- (my_diapo_simple_arrowHeight + ARROW_TIP + my_diapo_simple_lineWidth);  // hauteur du cadre avec les rayons et sans la pointe.
	
	const gdouble arrow_dec = 2;
	double fTotalWidth = fFrameWidth + 2 * fRadius;
	double w = fFrameWidth / fTotalWidth / 2;
	double h = MAX (0, fFrameHeight - 2 * fRadius) / fFrameHeight / 2;
	double rw = fRadius / fTotalWidth;
	double rh = fRadius / fFrameHeight;
	int i=0, t;
	int iPrecision = DELTA_ROUND_DEGREE;
	double x,y;  // 1ere coordonnee de la pointe.
	
	for (t = 0;t <= 90;t += iPrecision, i++) // cote haut droit.
	{
		_cairo_dock_set_vertex_xy (i,
			w + rw * cos (t*RADIAN),
			h + rh * sin (t*RADIAN));
	}
	if (!pDock->container.bDirectionUp && pDock->container.bIsHorizontal)  // dessin de la pointe vers le haut.
	{
		x = 0. + my_diapo_simple_arrowShift * (fFrameWidth/2 - my_diapo_simple_arrowWidth/2)/fTotalWidth + my_diapo_simple_arrowWidth/2/fTotalWidth;
		y = h + rh;
		_cairo_dock_set_vertex_xy (i,
			x,
			y);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x - my_diapo_simple_arrowWidth/2 * (1 + a * my_diapo_simple_arrowShift)/fTotalWidth,
			y + my_diapo_simple_arrowHeight/fFrameHeight);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x - my_diapo_simple_arrowWidth/fTotalWidth,
			y);
		i ++;
	}
	for (t = 90;t <= 180;t += iPrecision, i++) // haut gauche.
	{
		_cairo_dock_set_vertex_xy (i,
			-w + rw * cos (t*RADIAN),
			h + rh * sin (t*RADIAN));
	}
	if (!pDock->container.bDirectionUp && !pDock->container.bIsHorizontal)  // dessin de la pointe vers la gauche.
	{
		x = -w - rw;
		y = 0. + my_diapo_simple_arrowShift * (fFrameHeight/2 - fRadius - my_diapo_simple_arrowWidth/2)/fFrameHeight + my_diapo_simple_arrowWidth/2/fFrameHeight;
		_cairo_dock_set_vertex_xy (i,
			x,
			y);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x - my_diapo_simple_arrowHeight/fFrameHeight,
			y - my_diapo_simple_arrowWidth/2 * (1 + a * my_diapo_simple_arrowShift)/fFrameHeight);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x,
			y - my_diapo_simple_arrowWidth/fFrameHeight);
		i ++;
	}
	for (t = 180;t <= 270;t += iPrecision, i++) // bas gauche.
	{
		_cairo_dock_set_vertex_xy (i,
			-w + rw * cos (t*RADIAN),
			-h + rh * sin (t*RADIAN));
	}
	if (pDock->container.bDirectionUp && pDock->container.bIsHorizontal)  // dessin de la pointe vers le bas.
	{
		x = 0. + my_diapo_simple_arrowShift * (fFrameWidth/2 - my_diapo_simple_arrowWidth/2)/fTotalWidth - my_diapo_simple_arrowWidth/2/fTotalWidth;
		y = - h - rh;
		_cairo_dock_set_vertex_xy (i,
			x,
			y);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x + my_diapo_simple_arrowWidth/2 * (1 - a * my_diapo_simple_arrowShift)/fTotalWidth,
			y - my_diapo_simple_arrowHeight/fFrameHeight);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x + my_diapo_simple_arrowWidth/fTotalWidth,
			y);
		i ++;
	}
	for (t = 270;t <= 360;t += iPrecision, i++) // bas droit.
	{
		_cairo_dock_set_vertex_xy (i,
			w + rw * cos (t*RADIAN),
			-h + rh * sin (t*RADIAN));
	}
	if (pDock->container.bDirectionUp && !pDock->container.bIsHorizontal)  // dessin de la pointe vers la droite.
	{
		x = w + rw;
		y = 0. + my_diapo_simple_arrowShift * (fFrameHeight/2 - fRadius - my_diapo_simple_arrowWidth/2)/fFrameHeight - my_diapo_simple_arrowWidth/2/fFrameHeight;
		_cairo_dock_set_vertex_xy (i,
			x,
			y);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x + my_diapo_simple_arrowHeight/fFrameHeight,
			y + my_diapo_simple_arrowWidth/2 * (1 - a * my_diapo_simple_arrowShift)/fFrameHeight);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x,
			y + my_diapo_simple_arrowWidth/fFrameHeight);
		i ++;
	}
	_cairo_dock_close_path(i);
	
	*iNbPoints = i+1;
	_cairo_dock_return_vertex_tab ();
}

GLfloat *cd_rendering_generate_path_for_diapo_simple_opengl_without_arrow (CairoDock *pDock, double fAlpha, GLfloat **colors, int *iNbPoints)
{
	//static GLfloat pVertexTab[((90/DELTA_ROUND_DEGREE+1)*4+1+3)*3];  // +3 pour la pointe.
	_cairo_dock_define_static_vertex_tab ((90/DELTA_ROUND_DEGREE+1)*4+1+3);  // +3 pour la pointe.
	static GLfloat pColorTab[((90/DELTA_ROUND_DEGREE+1)*4+1+3)*4];  // +3 pour la pointe.
	double fRadius = my_diapo_simple_radius;
	double fFrameWidth  = pDock->iMaxDockWidth - 2*X_BORDER_SPACE - 2*fRadius;  // longueur du trait horizontal.
	double fFrameHeight = pDock->iMaxDockHeight- (my_diapo_simple_arrowHeight + ARROW_TIP + my_diapo_simple_lineWidth);  // hauteur du cadre avec les rayons et sans la pointe.
	
	const gdouble arrow_dec = 2;
	double fTotalWidth = fFrameWidth + 2 * fRadius;
	double w = fFrameWidth / fTotalWidth / 2;
	double h = MAX (0, fFrameHeight - 2 * fRadius) / fFrameHeight / 2;
	double rw = fRadius / fTotalWidth;
	double rh = fRadius / fFrameHeight;
	int i=0, t;
	int iPrecision = DELTA_ROUND_DEGREE;
	
	double *pTopRightColor, *pTopLeftColor, *pBottomLeftColor, *pBottomRightColor;
	double pMeanColor[4] = {(my_diapo_simple_color_frame_start[0] + my_diapo_simple_color_frame_stop[0])/2,
		(my_diapo_simple_color_frame_start[1] + my_diapo_simple_color_frame_stop[1])/2,
		(my_diapo_simple_color_frame_start[2] + my_diapo_simple_color_frame_stop[2])/2,
		(my_diapo_simple_color_frame_start[3] + my_diapo_simple_color_frame_stop[3])/2};
	pTopLeftColor = my_diapo_simple_color_frame_start;
	if (my_diapo_simple_fade2bottom || my_diapo_simple_fade2right)
	{
		pBottomRightColor = my_diapo_simple_color_frame_stop;
		if (my_diapo_simple_fade2bottom && my_diapo_simple_fade2right)
		{
			pBottomLeftColor = pMeanColor;
			pTopRightColor = pMeanColor;
		}
		else if (my_diapo_simple_fade2bottom)
		{
			pBottomLeftColor = my_diapo_simple_color_frame_stop;
			pTopRightColor = my_diapo_simple_color_frame_start;
		}
		else
		{
			pBottomLeftColor = my_diapo_simple_color_frame_start;
			pTopRightColor = my_diapo_simple_color_frame_stop;
		}
	}
	else
	{
		pBottomRightColor = my_diapo_simple_color_frame_start;
		pBottomLeftColor = my_diapo_simple_color_frame_start;
		pTopRightColor = my_diapo_simple_color_frame_start;
	}
	
	for (t = 0;t <= 90;t += iPrecision, i++) // cote haut droit.
	{
		_cairo_dock_set_vertex_xy (i,
			w + rw * cos (t*RADIAN),
			h + rh * sin (t*RADIAN));
		//pVertexTab[3*i] = w + rw * cos (t*RADIAN);
		//pVertexTab[3*i+1] = h + rh * sin (t*RADIAN);
		_copy_color (pColorTab, i, fAlpha, pTopRightColor);
	}
	for (t = 90;t <= 180;t += iPrecision, i++) // haut gauche.
	{
		_cairo_dock_set_vertex_xy (i,
			-w + rw * cos (t*RADIAN),
			h + rh * sin (t*RADIAN));
		//pVertexTab[3*i] = -w + rw * cos (t*RADIAN);
		//pVertexTab[3*i+1] = h + rh * sin (t*RADIAN);
		_copy_color (pColorTab, i, fAlpha, pTopLeftColor);
	}
	for (t = 180;t <= 270;t += iPrecision, i++) // bas gauche.
	{
		_cairo_dock_set_vertex_xy (i,
			-w + rw * cos (t*RADIAN),
			-h + rh * sin (t*RADIAN));
		//pVertexTab[3*i] = -w + rw * cos (t*RADIAN);
		//pVertexTab[3*i+1] = -h + rh * sin (t*RADIAN);
		_copy_color (pColorTab, i, fAlpha, pBottomLeftColor);
	}
	for (t = 270;t <= 360;t += iPrecision, i++) // bas droit.
	{
		_cairo_dock_set_vertex_xy (i,
			w + rw * cos (t*RADIAN),
			-h + rh * sin (t*RADIAN));
		//pVertexTab[3*i] = w + rw * cos (t*RADIAN);
		//pVertexTab[3*i+1] = -h + rh * sin (t*RADIAN);
		_copy_color (pColorTab, i, fAlpha, pBottomRightColor);
	}
	_cairo_dock_close_path(i);
	//pVertexTab[3*i] = w + rw;  // on boucle.
	//pVertexTab[3*i+1] = h;
	memcpy (&pColorTab[4*i], &pColorTab[0], 4*sizeof (GLfloat));
	
	*iNbPoints = i+1;
	*colors = pColorTab;
	_cairo_dock_return_vertex_tab ();
}

#define _set_arrow_color(c1, c2, f, fAlpha, colors) do {\
	colors[0] = c1[0] * (f) + c2[0] * (1 - (f));\
	colors[1] = c1[1] * (f) + c2[1] * (1 - (f));\
	colors[2] = c1[2] * (f) + c2[2] * (1 - (f));\
	colors[3] = (c1[3] * (f) + c2[3] * (1 - (f))) * fAlpha; } while (0)
GLfloat *cd_rendering_generate_arrow_path_for_diapo_simple_opengl (CairoDock *pDock, double fAlpha, GLfloat *color)
{
	//static GLfloat pVertexTab[((90/DELTA_ROUND_DEGREE+1)*4+1+3)*3];  // +3 pour la pointe.
	_cairo_dock_define_static_vertex_tab ((90/DELTA_ROUND_DEGREE+1)*4+1+3);  // +3 pour la pointe.
	double fRadius = my_diapo_simple_radius;
	double fFrameWidth  = pDock->iMaxDockWidth - 2*X_BORDER_SPACE - 2*fRadius;  // longueur du trait horizontal.
	double fFrameHeight = pDock->iMaxDockHeight- (my_diapo_simple_arrowHeight + ARROW_TIP + my_diapo_simple_lineWidth);  // hauteur du cadre avec les rayons et sans la pointe.
	
	const gdouble arrow_dec = 2;
	double fTotalWidth = fFrameWidth + 2 * fRadius;
	double w = fFrameWidth / fTotalWidth / 2;
	double h = MAX (0, fFrameHeight - 2 * fRadius) / fFrameHeight / 2;
	double rw = fRadius / fTotalWidth;
	double rh = fRadius / fFrameHeight;
	int i=0, t;
	int iPrecision = DELTA_ROUND_DEGREE;
	
	double *pTopRightColor, *pTopLeftColor, *pBottomLeftColor, *pBottomRightColor;
	double pMeanColor[4] = {(my_diapo_simple_color_frame_start[0] + my_diapo_simple_color_frame_stop[0])/2,
		(my_diapo_simple_color_frame_start[1] + my_diapo_simple_color_frame_stop[1])/2,
		(my_diapo_simple_color_frame_start[2] + my_diapo_simple_color_frame_stop[2])/2,
		(my_diapo_simple_color_frame_start[3] + my_diapo_simple_color_frame_stop[3])/2};
	pTopLeftColor = my_diapo_simple_color_frame_start;
	if (my_diapo_simple_fade2bottom || my_diapo_simple_fade2right)
	{
		pBottomRightColor = my_diapo_simple_color_frame_stop;
		if (my_diapo_simple_fade2bottom && my_diapo_simple_fade2right)
		{
			pBottomLeftColor = pMeanColor;
			pTopRightColor = pMeanColor;
		}
		else if (my_diapo_simple_fade2bottom)
		{
			pBottomLeftColor = my_diapo_simple_color_frame_stop;
			pTopRightColor = my_diapo_simple_color_frame_start;
		}
		else
		{
			pBottomLeftColor = my_diapo_simple_color_frame_start;
			pTopRightColor = my_diapo_simple_color_frame_stop;
		}
	}
	else
	{
		pBottomRightColor = my_diapo_simple_color_frame_start;
		pBottomLeftColor = my_diapo_simple_color_frame_start;
		pTopRightColor = my_diapo_simple_color_frame_start;
	}
	
	double x,y;  // 1ere coordonnee de la pointe.
	if (!pDock->container.bDirectionUp && pDock->container.bIsHorizontal)  // dessin de la pointe vers le haut.
	{
		x = 0. + my_diapo_simple_arrowShift * (fFrameWidth/2 - my_diapo_simple_arrowWidth/2)/fTotalWidth + my_diapo_simple_arrowWidth/2/fTotalWidth;
		y = h + rh;
		_cairo_dock_set_vertex_xy (i,
			x,
			y);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x - my_diapo_simple_arrowWidth/2 * (1 + a * my_diapo_simple_arrowShift)/fTotalWidth,
			y + my_diapo_simple_arrowHeight/fFrameHeight);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x - my_diapo_simple_arrowWidth/fTotalWidth,
			y);
		i ++;
		_set_arrow_color (pTopRightColor, pTopLeftColor, .5+my_diapo_simple_arrowShift/2, fAlpha, color);
	}
	else if (!pDock->container.bDirectionUp && !pDock->container.bIsHorizontal)  // dessin de la pointe vers la gauche.
	{
		x = -w - rw;
		y = 0. + my_diapo_simple_arrowShift * (fFrameHeight/2 - fRadius - my_diapo_simple_arrowWidth/2)/fFrameHeight + my_diapo_simple_arrowWidth/2/fFrameHeight;
		_cairo_dock_set_vertex_xy (i,
			x,
			y);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x - my_diapo_simple_arrowHeight/fFrameHeight,
			y - my_diapo_simple_arrowWidth/2 * (1 + a * my_diapo_simple_arrowShift)/fFrameHeight);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x,
			y - my_diapo_simple_arrowWidth/fFrameHeight);
		i ++;
		_set_arrow_color (pTopLeftColor, pBottomLeftColor, .5+my_diapo_simple_arrowShift/2, fAlpha, color);
	}
	else if (pDock->container.bDirectionUp && pDock->container.bIsHorizontal)  // dessin de la pointe vers le bas.
	{
		x = 0. + my_diapo_simple_arrowShift * (fFrameWidth/2 - my_diapo_simple_arrowWidth/2)/fTotalWidth - my_diapo_simple_arrowWidth/2/fTotalWidth;
		y = - h - rh;
		_cairo_dock_set_vertex_xy (i,
			x,
			y);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x + my_diapo_simple_arrowWidth/2 * (1 - a * my_diapo_simple_arrowShift)/fTotalWidth,
			y - my_diapo_simple_arrowHeight/fFrameHeight);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x + my_diapo_simple_arrowWidth/fTotalWidth,
			y);
		i ++;
		_set_arrow_color (pBottomRightColor, pBottomLeftColor, .5+my_diapo_simple_arrowShift/2, fAlpha, color);
	}
	else if (pDock->container.bDirectionUp && !pDock->container.bIsHorizontal)  // dessin de la pointe vers la droite.
	{
		x = w + rw;
		y = 0. + my_diapo_simple_arrowShift * (fFrameHeight/2 - fRadius - my_diapo_simple_arrowWidth/2)/fFrameHeight - my_diapo_simple_arrowWidth/2/fFrameHeight;
		_cairo_dock_set_vertex_xy (i,
			x,
			y);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x + my_diapo_simple_arrowHeight/fFrameHeight,
			y + my_diapo_simple_arrowWidth/2 * (1 - a * my_diapo_simple_arrowShift)/fFrameHeight);
		i ++;
		_cairo_dock_set_vertex_xy (i,
			x,
			y + my_diapo_simple_arrowWidth/fFrameHeight);
		i ++;
		_set_arrow_color (pTopRightColor, pBottomRightColor, .5+my_diapo_simple_arrowShift/2, fAlpha, color);
	}
	_cairo_dock_close_path (i);  // on boucle.
	_cairo_dock_return_vertex_tab ();
}
