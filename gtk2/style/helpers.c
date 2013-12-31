/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
 *   Copyright 2013 - 2013 Yichao Yu <yyc1992@gmail.com>                     *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU Lesser General Public License as          *
 *   published by the Free Software Foundation; either version 2.1 of the    *
 *   License, or (at your option) version 3, or any later version accepted   *
 *   by the membership of KDE e.V. (or its successor approved by the         *
 *   membership of KDE e.V.), which shall act as a proxy defined in          *
 *   Section 6 of version 3 of the license.                                  *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 *   Lesser General Public License for more details.                         *
 *                                                                           *
 *   You should have received a copy of the GNU Lesser General Public        *
 *   License along with this library. If not,                                *
 *   see <http://www.gnu.org/licenses/>.                                     *
 *****************************************************************************/

#include <qtcurve-utils/gtkutils.h>
#include <qtcurve-utils/x11qtc.h>
#include <qtcurve-utils/x11blur.h>
#include <qtcurve-utils/color.h>
#include <qtcurve-utils/log.h>

#include "helpers.h"
#include "qt_settings.h"
#include <gdk/gdkx.h>

void
debugDisplayWidget(GtkWidget *widget, int level)
{
    if (qtcLogLevel > QTC_LOG_DEBUG)
        return;
    if (level < 0 || !widget) {
        printf("\n");
        return;
    }
    const char *type_name = g_type_name(G_OBJECT_TYPE(widget));
    const char *widget_name = gtk_widget_get_name(widget);
    qtcDebug("%s(%s)[%p] ", type_name, widget_name ? widget_name : "NULL",
             widget);
    debugDisplayWidget(gtk_widget_get_parent(widget), level - 1);
}

bool
haveAlternateListViewCol()
{
    return (qtSettings.colors[PAL_ACTIVE][COLOR_LV].red != 0 ||
            qtSettings.colors[PAL_ACTIVE][COLOR_LV].green != 0 ||
            qtSettings.colors[PAL_ACTIVE][COLOR_LV].blue != 0);
}

GdkColor*
menuColors(gboolean active)
{
    return (SHADE_WINDOW_BORDER == opts.shadeMenubars ?
            qtcPalette.wborder[active ? 1 : 0] :
            SHADE_NONE == opts.shadeMenubars ||
            (opts.shadeMenubarOnlyWhenActive && !active) ?
            qtcPalette.background : qtcPalette.menubar);
}

EBorder
shadowToBorder(GtkShadowType shadow)
{
    switch (shadow) {
    default:
    case GTK_SHADOW_NONE:
        return BORDER_FLAT;
    case GTK_SHADOW_IN:
    case GTK_SHADOW_ETCHED_IN:
        return BORDER_SUNKEN;
    case GTK_SHADOW_OUT:
    case GTK_SHADOW_ETCHED_OUT:
        return BORDER_RAISED;
    }
}

gboolean
useButtonColor(const char *detail)
{
    return (detail &&
            (strcmp(detail, "optionmenu") == 0 ||
             strcmp(detail, "button") == 0 ||
             strcmp(detail, "buttondefault") == 0 ||
             strcmp(detail, "togglebuttondefault") == 0 ||
             strcmp(detail, "togglebutton") == 0 ||
             strcmp(detail, "hscale") == 0 ||
             strcmp(detail, "vscale") == 0 ||
             strcmp(detail, "spinbutton") == 0 ||
             strcmp(detail, "spinbutton_up") == 0 ||
             strcmp(detail, "spinbutton_down") == 0 ||
             strcmp(detail, "slider") == 0 ||
             strcmp(detail, "qtc-slider") == 0 ||
             (detail[0] && strncmp(detail + 1, "scrollbar",
                                   strlen("scrollbar")) == 0) ||
             strcmp(detail, "stepper") == 0));
}

void
qtcShadeColors(GdkColor *base, GdkColor *vals)
{
    gboolean useCustom = USE_CUSTOM_SHADES(opts);
    double hl = TO_FACTOR(opts.highlightFactor);

    for (int i = 0;i < QTC_NUM_STD_SHADES;i++) {
        qtcShade(base, &vals[i], useCustom ? opts.customShades[i] :
                 qtcShadeGetIntern(opts.contrast, i, opts.darkerBorders,
                                   opts.shading), opts.shading);
    }
    qtcShade(base, &vals[SHADE_ORIG_HIGHLIGHT], hl, opts.shading);
    qtcShade(&vals[4], &vals[SHADE_4_HIGHLIGHT], hl, opts.shading);
    qtcShade(&vals[2], &vals[SHADE_2_HIGHLIGHT], hl, opts.shading);
    vals[ORIGINAL_SHADE] = *base;
}

gboolean
isSortColumn(GtkWidget *button)
{
    GtkWidget *parent = NULL;
    if (button && (parent = gtk_widget_get_parent(button)) &&
        GTK_IS_TREE_VIEW(parent)) {
#if GTK_CHECK_VERSION(2, 90, 0)
        GtkWidget *box = (GTK_IS_BUTTON(button) ?
                          gtk_bin_get_child(GTK_BIN(button)) : NULL);

        if (box && GTK_IS_BOX(box)) {
            GList *children = gtk_container_get_children(GTK_CONTAINER(box));
            gboolean found = FALSE;

            for (GList *child = children;child && !found;
                 child = g_list_next(child)) {
                if (GTK_IS_ARROW(child->data)) {
                    int val;
                    g_object_get(child->data, "arrow-type", &val, NULL);
                    if (GTK_ARROW_NONE != val) {
                        found = TRUE;
                    }
                }
            }

            if (children)
                g_list_free(children);
            return found;
        }
#else
        GtkWidget *sort = NULL;
        GList *columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(parent));

        for (GList *column = columns;column && !sort && sort != button;
             column = g_list_next(column)) {
            if (GTK_IS_TREE_VIEW_COLUMN(column->data)) {
                GtkTreeViewColumn *c = GTK_TREE_VIEW_COLUMN(column->data);
                if (gtk_tree_view_column_get_sort_indicator(c)) {
                    sort = c->button;
                }
            }
        }

        if (columns)
            g_list_free(columns);
        return sort == button;
#endif
    }
    return FALSE;
};

GdkColor*
getCellCol(GdkColor *std, const char *detail)
{
    static GdkColor shaded;

    if (!qtSettings.shadeSortedList || !strstr(detail, "_sorted"))
        return std;

    shaded = *std;

    if (IS_BLACK(shaded)) {
        shaded.red = shaded.green = shaded.blue = 55 << 8;
    } else {
        double r = shaded.red / 65535.0;
        double g = shaded.green / 65535.0;
        double b = shaded.blue / 65535.0;
        double h, s, v;

        qtcRgbToHsv(r, g, b, &h, &s, &v);

        if (v > 175.0 / 255.0) {
            v *= 100.0 / 104.0;
        } else {
            v *= 120.0 / 100.0;
        }

        if (v > 1.0) {
            s -= v - 1.0;
            if (s < 0)
                s = 0;
            v = 1.0;
        }

        qtcHsvToRgb(&r, &g, &b, h, s, v);
        shaded.red = r * 65535.0;
        shaded.green = g * 65535.0;
        shaded.blue = b * 65535.0;
    }
    return &shaded;
}

gboolean
reverseLayout(GtkWidget *widget)
{
    if (widget)
        return GTK_TEXT_DIR_RTL == gtk_widget_get_direction(widget);
    return FALSE;
}

gboolean
isOnToolbar(GtkWidget *widget, gboolean *horiz, int level)
{
    if (widget) {
        if (GTK_IS_TOOLBAR(widget)) {
            if (horiz)
                *horiz = qtcWidgetIsHorizontal(widget);
            return TRUE;
        } else if (level < 4) {
            return isOnToolbar(gtk_widget_get_parent(widget), horiz, level + 1);
        }
    }
    return FALSE;
}

gboolean
isOnHandlebox(GtkWidget *widget, gboolean *horiz, int level)
{
    if (widget) {
        if (GTK_IS_HANDLE_BOX(widget)) {
            if (horiz) {
                GtkPositionType pos =
                    gtk_handle_box_get_handle_position(GTK_HANDLE_BOX(widget));
                *horiz = (GTK_POS_LEFT == pos || GTK_POS_RIGHT == pos);
            }
            return TRUE;
        } else if (level < 4) {
            return isOnHandlebox(gtk_widget_get_parent(widget), horiz,
                                 level + 1);
        }
    }
    return FALSE;
}

gboolean
isButtonOnToolbar(GtkWidget *widget, gboolean *horiz)
{
    GtkWidget *parent = NULL;
    if (widget && (parent = gtk_widget_get_parent(widget)) &&
        GTK_IS_BUTTON(widget))
        return isOnToolbar(parent, horiz, 0);
    return FALSE;
}

gboolean
isButtonOnHandlebox(GtkWidget *widget, gboolean *horiz)
{
    GtkWidget *parent = NULL;
    if (widget && (parent = gtk_widget_get_parent(widget)) &&
        GTK_IS_BUTTON(widget))
        return isOnHandlebox(parent, horiz, 0);
    return FALSE;
}

gboolean
isOnStatusBar(GtkWidget *widget, int level)
{
    GtkWidget *parent = gtk_widget_get_parent(widget);
    if (parent) {
        if (GTK_IS_STATUSBAR(parent)) {
            return TRUE;
        } else if (level < 4) {
            return isOnStatusBar(parent, level + 1);
        }
    }
    return FALSE;
}

gboolean isList(GtkWidget *widget)
{
    return widget &&
           (GTK_IS_TREE_VIEW(widget) ||
#if !GTK_CHECK_VERSION(2, 90, 0)
            GTK_IS_CLIST(widget) ||
            GTK_IS_LIST(widget) ||

#ifdef GTK_ENABLE_BROKEN
            GTK_IS_TREE(widget) ||
#endif
            GTK_IS_CTREE(widget) ||
#endif
            0 == strcmp(g_type_name(G_OBJECT_TYPE(widget)), "GtkSCTree"));
}

gboolean isListViewHeader(GtkWidget *widget)
{
    GtkWidget *parent=NULL;
    return widget && GTK_IS_BUTTON(widget) && (parent=gtk_widget_get_parent(widget)) &&
           (isList(parent) ||
            (GTK_APP_GIMP == qtSettings.app && GTK_IS_BOX(parent) &&
             (parent=gtk_widget_get_parent(parent)) && GTK_IS_EVENT_BOX(parent) &&
             (parent=gtk_widget_get_parent(parent)) && 0 == strcmp(g_type_name(G_OBJECT_TYPE(parent)), "GimpThumbBox")));
}

gboolean isEvolutionListViewHeader(GtkWidget *widget, const char *detail)
{
    GtkWidget *parent=NULL;
    return GTK_APP_EVOLUTION == qtSettings.app && widget && DETAIL("button") &&
           0 == strcmp(g_type_name(G_OBJECT_TYPE(widget)), "ECanvas") &&
           (parent=gtk_widget_get_parent(widget)) && (parent=gtk_widget_get_parent(parent)) &&
           GTK_IS_SCROLLED_WINDOW(parent);
}

gboolean isOnListViewHeader(GtkWidget *w, int level)
{
    if(w)
    {
        if(isListViewHeader(w))
            return TRUE;
        else if(level<4)
            return isOnListViewHeader(gtk_widget_get_parent(w), ++level);
    }
    return FALSE;
}

gboolean isPathButton(GtkWidget *widget)
{
    return widget && gtk_widget_get_parent(widget) && GTK_IS_BUTTON(widget) &&
           0 == strcmp(g_type_name(G_OBJECT_TYPE(gtk_widget_get_parent(widget))), "GtkPathBar");
}

// static gboolean isTabButton(GtkWidget *widget)
// {
//     return widget && GTK_IS_BUTTON(widget) && gtk_widget_get_parent(widget) &&
//            (GTK_IS_NOTEBOOK(gtk_widget_get_parent(widget)) ||
//             (gtk_widget_get_parent(widget)->parent && GTK_IS_BOX(gtk_widget_get_parent(widget)) && GTK_IS_NOTEBOOK(gtk_widget_get_parent(widget)->parent)));
// }

GtkWidget * getComboEntry(GtkWidget *widget)
{
    GList     *children = gtk_container_get_children(GTK_CONTAINER(widget)),
              *child    = children;
    GtkWidget *rv       = NULL;

    for(; child && !rv; child=child->next)
    {
        GtkWidget *boxChild=(GtkWidget *)child->data;

        if(GTK_IS_ENTRY(boxChild))
            rv=(GtkWidget *)boxChild;
    }

    if(children)
        g_list_free(children);
    return rv;
}

GtkWidget * getComboButton(GtkWidget *widget)
{
    GList     *children = gtk_container_get_children(GTK_CONTAINER(widget)),
              *child    = children;
    GtkWidget *rv       = NULL;

    for(; child && !rv; child=child->next)
    {
        GtkWidget *boxChild=(GtkWidget *)child->data;

        if(GTK_IS_BUTTON(boxChild))
            rv=(GtkWidget *)boxChild;
    }

    if(children)
        g_list_free(children);
    return rv;
}

gboolean isSideBarBtn(GtkWidget *widget)
{
    GtkWidget *parent = NULL;
    return widget && (parent=gtk_widget_get_parent(widget)) &&
           (0 == strcmp(g_type_name(G_OBJECT_TYPE(parent)), "GdlDockBar") ||
            (0 == strcmp(g_type_name(G_OBJECT_TYPE(parent)), "GdlSwitcher")/* &&
             gtk_widget_get_parent(parent) &&
             0 == strcmp(g_type_name(G_OBJECT_TYPE(parent)), "GdlDockNotebook")*/) );
}

gboolean isComboBoxButton(GtkWidget *widget)
{
    GtkWidget *parent=NULL;
    return widget && GTK_IS_BUTTON(widget) && (parent=gtk_widget_get_parent(widget)) &&
           (QTC_COMBO_ENTRY(parent) || QTC_IS_COMBO(parent));
}

gboolean isComboBox(GtkWidget *widget)
{
    GtkWidget *parent=NULL;
    return widget && GTK_IS_BUTTON(widget) && (parent=gtk_widget_get_parent(widget)) &&
           !QTC_COMBO_ENTRY(parent) && (GTK_IS_COMBO_BOX(parent) || QTC_IS_COMBO(parent));
}

gboolean isComboBoxEntry(GtkWidget *widget)
{
    GtkWidget *parent=NULL;
    return widget && GTK_IS_ENTRY(widget) && (parent=gtk_widget_get_parent(widget)) &&
           (QTC_COMBO_ENTRY(parent) || QTC_IS_COMBO(parent));
}

gboolean isComboBoxEntryButton(GtkWidget *widget)
{
    GtkWidget *parent=NULL;
    return widget && (parent=gtk_widget_get_parent(widget)) && GTK_IS_TOGGLE_BUTTON(widget) && QTC_COMBO_ENTRY(parent);
}

/*
static gboolean isSwtComboBoxEntry(GtkWidget *widget)
{
    return GTK_APP_JAVA_SWT == qtSettings.app &&
           isComboBoxEntry(widget) &&
           gtk_widget_get_parent(widget)->parent && 0 == strcmp(g_type_name(G_OBJECT_TYPE(gtk_widget_get_parent(widget)->parent)), "SwtFixed");
}
*/

gboolean isGimpCombo(GtkWidget *widget)
{
    GtkWidget *parent=NULL;
    return GTK_APP_GIMP == qtSettings.app &&
           widget && (parent=gtk_widget_get_parent(widget)) && GTK_IS_TOGGLE_BUTTON(widget) &&
           0 == strcmp(g_type_name(G_OBJECT_TYPE(parent)), "GimpEnumComboBox");
}

gboolean isOnComboEntry(GtkWidget *w, int level)
{
    if(w)
    {
        if(QTC_COMBO_ENTRY(w))
            return TRUE;
        else if(level<4)
            return isOnComboEntry(gtk_widget_get_parent(w), ++level);
    }
    return FALSE;
}

gboolean isOnComboBox(GtkWidget *w, int level)
{
    if(w)
    {
        if(GTK_IS_COMBO_BOX(w))
            return TRUE;
        else if(level<4)
            return isOnComboBox(gtk_widget_get_parent(w), ++level);
    }
    return FALSE;
}

gboolean
isOnCombo(GtkWidget *w, int level)
{
    if (w) {
        if (QTC_IS_COMBO(w)) {
            return TRUE;
        } else if(level < 4) {
            return isOnCombo(gtk_widget_get_parent(w), ++level);
        }
    }
    return FALSE;
}

#if !GTK_CHECK_VERSION(2, 90, 0)
gboolean isOnOptionMenu(GtkWidget *w, int level)
{
    if (w) {
        if(GTK_IS_OPTION_MENU(w))
            return TRUE;
        else if(level<4)
            return isOnOptionMenu(gtk_widget_get_parent(w), ++level);
    }
    return FALSE;
}

gboolean isActiveOptionMenu(GtkWidget *widget)
{
    if(GTK_IS_OPTION_MENU(widget))
    {
        GtkWidget *menu=gtk_option_menu_get_menu(GTK_OPTION_MENU(widget));
        if(menu && gtk_widget_get_visible(menu) && gtk_widget_get_realized(menu))
            return TRUE;
    }
    return FALSE;
}
#endif

gboolean isOnMenuItem(GtkWidget *w, int level)
{
    if(w)
    {
        if(GTK_IS_MENU_ITEM(w))
            return TRUE;
        else if(level<4)
            return isOnMenuItem(gtk_widget_get_parent(w), ++level);
    }
    return FALSE;
}

gboolean isSpinButton(GtkWidget *widget)
{
    return widget && GTK_IS_SPIN_BUTTON(widget);
}

gboolean isStatusBarFrame(GtkWidget *widget)
{
    GtkWidget *parent=NULL;
    return widget && (parent=gtk_widget_get_parent(widget)) && GTK_IS_FRAME(widget) &&
           (GTK_IS_STATUSBAR(parent) || ((parent=gtk_widget_get_parent(parent)) && GTK_IS_STATUSBAR(parent)));
}

GtkMenuBar * isMenubar(GtkWidget *w, int level)
{
    if(w)
    {
        if(GTK_IS_MENU_BAR(w))
            return (GtkMenuBar*)w;
        else if(level<3)
            return isMenubar(gtk_widget_get_parent(w), level++);
    }

    return NULL;
}

gboolean isMenuitem(GtkWidget *w, int level)
{
    if(w)
    {
        if(GTK_IS_MENU_ITEM(w))
            return TRUE;
        else if(level<3)
            return isMenuitem(gtk_widget_get_parent(w), level++);
    }

    return FALSE;
}

gboolean isMenuWindow(GtkWidget *w)
{
    GtkWidget *def = gtk_window_get_default_widget(GTK_WINDOW(w));

    return def && GTK_IS_MENU(def);
}

gboolean isInGroupBox(GtkWidget *w, int level)
{
    if(w)
    {
        if(IS_GROUP_BOX(w))
            return TRUE;
        else if(level<5)
            return isInGroupBox(gtk_widget_get_parent(w), level++);
    }

    return FALSE;
}

gboolean isOnButton(GtkWidget *w, int level, gboolean *def)
{
    if(w)
    {
        if((GTK_IS_BUTTON(w)
#if !GTK_CHECK_VERSION(2, 90, 0)
            || GTK_IS_OPTION_MENU(w)
#endif
            ) && (!(GTK_IS_RADIO_BUTTON(w) || GTK_IS_CHECK_BUTTON(w))))
        {
            if(def)
                *def=gtk_widget_has_default(w);
            return TRUE;
        }
        else if(level<3)
            return isOnButton(gtk_widget_get_parent(w), level++, def);
    }

    return FALSE;
}

static GtkRequisition defaultOptionIndicatorSize    = { 6, 13 };
static GtkBorder      defaultOptionIndicatorSpacing = { 7, 5, 1, 1 };

void optionMenuGetProps(GtkWidget *widget, GtkRequisition *indicator_size, GtkBorder *indicator_spacing)
{
    GtkRequisition *tmp_size = NULL;
    GtkBorder      *tmp_spacing = NULL;

    if(widget)
        gtk_widget_style_get(widget, "indicator_size", &tmp_size, "indicator_spacing", &tmp_spacing,
                             NULL);
    *indicator_size= tmp_size ? *tmp_size : defaultOptionIndicatorSize;
    *indicator_spacing = tmp_spacing ? *tmp_spacing : defaultOptionIndicatorSpacing;

    if (tmp_size)
        gtk_requisition_free(tmp_size);
    if (tmp_spacing)
        gtk_border_free(tmp_spacing);
}

#if GTK_CHECK_VERSION(2, 90, 0)
EStepper getStepper(const char *detail)
#else
EStepper getStepper(GtkWidget *widget, int x, int y, int width, int height)
#endif
{
#if GTK_CHECK_VERSION(2, 90, 0)
    if(detail && detail[1])
    {
        if(0 == strcmp(&detail[11], "end_inner"))
            return STEPPER_C;
        else if(strstr(&detail[11], "start_inner"))
            return STEPPER_B;
        else if(0 == strcmp(&detail[11], "end"))
            return STEPPER_D;
        else if(strstr(&detail[11], "start"))
            return STEPPER_A;
    }
#else
    if(widget && GTK_IS_RANGE(widget))
    {
        GdkRectangle   tmp;
        GdkRectangle   check_rectangle,
                       stepper;
        GtkOrientation orientation = qtcWidgetGetOrientation(widget);
        GtkAllocation alloc = qtcWidgetGetAllocation(widget);

        stepper.x=x;
        stepper.y=y;
        stepper.width=width;
        stepper.height=height;
        check_rectangle.x      = alloc.x;
        check_rectangle.y      = alloc.y;
        check_rectangle.width  = stepper.width;
        check_rectangle.height = stepper.height;

        if (-1 == alloc.x && -1 == alloc.y)
            return STEPPER_NONE;

        if (gdk_rectangle_intersect(&stepper, &check_rectangle, &tmp))
            return STEPPER_A;

        if (GTK_ORIENTATION_HORIZONTAL == orientation)
            check_rectangle.x = alloc.x + stepper.width;
        else
            check_rectangle.y = alloc.y + stepper.height;

        if (gdk_rectangle_intersect(&stepper, &check_rectangle, &tmp))
            return STEPPER_B;

        if (GTK_ORIENTATION_HORIZONTAL == orientation)
            check_rectangle.x = alloc.x + alloc.width - (stepper.width * 2);
        else
            check_rectangle.y = alloc.y + alloc.height - (stepper.height * 2);

        if (gdk_rectangle_intersect(&stepper, &check_rectangle, &tmp))
            return STEPPER_C;

        if (GTK_ORIENTATION_HORIZONTAL == orientation)
            check_rectangle.x = alloc.x + alloc.width - stepper.width;
        else
            check_rectangle.y = alloc.y + alloc.height - stepper.height;

        if (gdk_rectangle_intersect(&stepper, &check_rectangle, &tmp))
            return STEPPER_D;
    }
#endif
    return STEPPER_NONE;
}

#if GTK_CHECK_VERSION(2, 90, 0)
void gdk_drawable_get_size(GdkWindow *window, int *width, int *height)
{
    *width=gdk_window_get_width(window);
    *height=gdk_window_get_height(window);
}

void sanitizeSizeReal(GtkWidget *widget, int *width, int *height)
{
    if(-1 == *width || -1 == *height)
    {
        GdkWindow *window=gtk_widget_get_window(widget);

        if(window)
        {
            if((-1 == *width) && (-1 == *height))
                gdk_drawable_get_size(window, width, height);
            else if(-1 == *width)
                gdk_drawable_get_size(window, width, NULL);
            else if(-1 == *height)
                gdk_drawable_get_size(window, NULL, height);
        }
    }
}
#else
void sanitizeSize(GdkWindow *window, int *width, int *height)
{
    if((-1 == *width) && (-1 == *height))
        gdk_window_get_size(window, width, height);
    else if(-1 == *width)
        gdk_window_get_size(window, width, NULL);
    else if(-1 == *height)
        gdk_window_get_size(window, NULL, height);
}
#endif

int getFillReal(GtkStateType state, gboolean set, gboolean darker)
{
    return GTK_STATE_INSENSITIVE == state
               ? (darker ? 2 : ORIGINAL_SHADE)
               : GTK_STATE_PRELIGHT == state
                   ? set /*&& allow_mouse_over_set*/
                       ? (darker ? 3 : SHADE_4_HIGHLIGHT)
                       : (darker ? SHADE_2_HIGHLIGHT : SHADE_ORIG_HIGHLIGHT)
                   : set || GTK_STATE_ACTIVE == state
                       ? (darker ? 5 : 4)
                       : (darker ? 2 : ORIGINAL_SHADE);
}

gboolean isSbarDetail(const char *detail)
{
    return detail && (
#if GTK_CHECK_VERSION(2, 90, 0)
                      (detail[1] && &detail[1] == strstr(detail, "scrollbar")) ||
#endif
                      0 == strcmp(detail, "hscrollbar") || 0 == strcmp(detail, "vscrollbar") || 0 == strcmp(detail, "stepper"));
}

int getRound(const char *detail, GtkWidget *widget, int x, int y, int width, int height, gboolean rev)
{
    if(detail)
    {
        if(0 == strcmp(detail, "slider"))
            return
#ifndef SIMPLE_SCROLLBARS
                    !(opts.square&SQUARE_SB_SLIDER) && (SCROLLBAR_NONE == opts.scrollbarType || opts.flatSbarButtons)
                        ? ROUNDED_ALL :
#endif
                    ROUNDED_NONE;
        else if(0 == strcmp(detail, "qtc-slider"))
            return opts.square&SQUARE_SLIDER && (SLIDER_PLAIN == opts.sliderStyle || SLIDER_PLAIN_ROTATED == opts.sliderStyle)
                ? ROUNDED_NONE : ROUNDED_ALL;
        else if(0 == strcmp(detail, "splitter") || 0 == strcmp(detail, "optionmenu")  ||
                0 == strcmp(detail, "togglebutton") || 0 == strcmp(detail, "hscale") ||
                0 == strcmp(detail, "vscale") )
            return ROUNDED_ALL;
        else if(0 == strcmp(detail, "spinbutton_up"))
            return rev ? ROUNDED_TOPLEFT : ROUNDED_TOPRIGHT;
        else if(0 == strcmp(detail, "spinbutton_down"))
            return rev ? ROUNDED_BOTTOMLEFT : ROUNDED_BOTTOMRIGHT;
        else if(isSbarDetail(detail))
        {

            switch(getStepper(
#if GTK_CHECK_VERSION(2, 90, 0)
                                detail
#else
                                widget, x, y, width, height
#endif
                              ))
            {
                case STEPPER_A:
                    return 'h' == detail[0] ? ROUNDED_LEFT : ROUNDED_TOP;
                case STEPPER_D:
                    return 'v' == detail[0] ? ROUNDED_BOTTOM : ROUNDED_RIGHT;
                default:
                    return ROUNDED_NONE;
            }
        }
        else if(0 == strcmp(detail, "button"))
        {
            if(isListViewHeader(widget))
                return ROUNDED_NONE;
            else if(isComboBoxButton(widget))
                return rev ? ROUNDED_LEFT : ROUNDED_RIGHT;
            else
                return ROUNDED_ALL;
        }
    }

    return ROUNDED_NONE;
}

gboolean
isHorizontalProgressbar(GtkWidget *widget)
{
    if (!widget || isMozilla() || !GTK_IS_PROGRESS_BAR(widget))
        return TRUE;
#if GTK_CHECK_VERSION(2, 90, 0)
    return qtcWidgetIsHorizontal(widget);
#else
    switch (GTK_PROGRESS_BAR(widget)->orientation) {
    default:
    case GTK_PROGRESS_LEFT_TO_RIGHT:
    case GTK_PROGRESS_RIGHT_TO_LEFT:
        return TRUE;
    case GTK_PROGRESS_BOTTOM_TO_TOP:
    case GTK_PROGRESS_TOP_TO_BOTTOM:
        return FALSE;
    }
#endif
}

gboolean isComboBoxPopupWindow(GtkWidget *widget, int level)
{
    if(widget)
    {
        if(gtk_widget_get_name(widget) && GTK_IS_WINDOW(widget) &&
           0 == strcmp(gtk_widget_get_name(widget), "gtk-combobox-popup-window"))
            return TRUE;
        else if(level<4)
            return isComboBoxPopupWindow(gtk_widget_get_parent(widget), ++level);
    }
    return FALSE;
}

gboolean isComboBoxList(GtkWidget *widget)
{
    GtkWidget *parent=NULL;
    return widget && (parent=gtk_widget_get_parent(widget)) && /*GTK_IS_FRAME(widget) && */isComboBoxPopupWindow(parent, 0);
}

gboolean isComboPopupWindow(GtkWidget *widget, int level)
{
    if(widget)
    {
        if(gtk_widget_get_name(widget) && GTK_IS_WINDOW(widget) &&
            0 == strcmp(gtk_widget_get_name(widget), "gtk-combo-popup-window"))
            return TRUE;
        else if(level<4)
            return isComboPopupWindow(gtk_widget_get_parent(widget), ++level);
    }
    return FALSE;
}

gboolean isComboList(GtkWidget *widget)
{
    GtkWidget *parent=NULL;
    return widget && (parent=gtk_widget_get_parent(widget)) && isComboPopupWindow(parent, 0);
}

gboolean isComboMenu(GtkWidget *widget)
{
    if(widget && gtk_widget_get_name(widget) && GTK_IS_MENU(widget) && 0 == strcmp(gtk_widget_get_name(widget), "gtk-combobox-popup-menu"))
        return TRUE;
    else
    {
        GtkWidget *top        = gtk_widget_get_toplevel(widget),
                  *topChild   = top ? gtk_bin_get_child(GTK_BIN(top)) : NULL,
                  *transChild = NULL;
        GtkWindow *trans      = NULL;

        return topChild && (isComboBoxPopupWindow(topChild, 0) ||
                       //GTK_IS_DIALOG(top) || /* Dialogs should not have menus! */
                       (GTK_IS_WINDOW(top) && (trans=gtk_window_get_transient_for(GTK_WINDOW(top))) &&
                        (transChild=gtk_bin_get_child(GTK_BIN(trans))) && isComboMenu(transChild)));
    }
}

gboolean isComboFrame(GtkWidget *widget)
{
    GtkWidget *parent=NULL;
    return !QTC_COMBO_ENTRY(widget) && GTK_IS_FRAME(widget) && (parent=gtk_widget_get_parent(widget)) && GTK_IS_COMBO_BOX(parent);
}

gboolean isFixedWidget(GtkWidget *widget)
{
    GtkWidget *parent=NULL;
    return widget && (parent=gtk_widget_get_parent(widget)) && GTK_IS_FIXED(parent) &&
           (parent=gtk_widget_get_parent(parent)) && GTK_IS_WINDOW(parent);
}

gboolean isGimpDockable(GtkWidget *widget)
{
    if(GTK_APP_GIMP == qtSettings.app)
    {
        GtkWidget *wid=widget;
        while(wid)
        {
            if(0 == strcmp(g_type_name(G_OBJECT_TYPE(wid)), "GimpDockable") ||
               0 == strcmp(g_type_name(G_OBJECT_TYPE(wid)), "GimpToolbox"))
                return TRUE;
            wid=gtk_widget_get_parent(wid);
        }
    }
    return FALSE;
}

GdkColor * getParentBgCol(GtkWidget *widget)
{
    if(GTK_IS_SCROLLBAR(widget))
        widget=gtk_widget_get_parent(widget);

    if(widget)
    {
        widget=gtk_widget_get_parent(widget);
        while(widget && GTK_IS_BOX(widget))
            widget=gtk_widget_get_parent(widget);
    }

    GtkStyle *style=widget ? gtk_widget_get_style(widget) : NULL;
    return style
               ? &(style->bg[gtk_widget_get_state(widget)])
               : NULL;
}

int
getOpacity(GtkWidget *widget)
{
    if (opts.bgndOpacity == opts.dlgOpacity)
        return opts.bgndOpacity;

    if (opts.bgndOpacity != 100 || opts.dlgOpacity != 100) {
        if (!widget) {
            return opts.bgndOpacity;
        } else {
            GtkWidget *top = gtk_widget_get_toplevel(widget);
            return (top && GTK_IS_DIALOG(top) ? opts.dlgOpacity :
                    opts.bgndOpacity);
        }
    }
    return 100;
}

gboolean eqRect(GdkRectangle *a, GdkRectangle *b)
{
    return a->x == b->x && a->y == b->y && a->width == b->width && a->height == b->height;
}

void setLowerEtchCol(cairo_t *cr, GtkWidget *widget)
{
    if(USE_CUSTOM_ALPHAS(opts))
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, opts.customAlphas[ALPHA_ETCH_LIGHT]);
    else if(qtcIsFlatBgnd(opts.bgndAppearance) && (!widget || !g_object_get_data(G_OBJECT (widget), "transparent-bg-hint")))
    {
        GdkColor *parentBg=getParentBgCol(widget);

        if (parentBg) {
            GdkColor col;
            qtcShade(parentBg, &col, 1.06, opts.shading);
            qtcCairoSetColor(cr, &col);
        } else {
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.1); // 0.25);
        }
    } else {
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.1); // 0.4);
    }
}

GdkColor
shadeColor(GdkColor *orig, double mod)
{
    if (!qtcEqual(mod, 0.0)) {
        GdkColor modified;
        qtcShade(orig, &modified, mod, opts.shading);
        return modified;
    }
    return *orig;
}

void
constrainRect(GdkRectangle *rect, GdkRectangle *con)
{
    if (rect && con) {
        if (rect->x < con->x) {
            rect->width -= con->x - rect->x;
            rect->x = con->x;
        }
        if(rect->y < con->y) {
            rect->height -= rect->y - con->y;
            rect->y = con->y;
        }
        if ((rect->x + rect->width) > (con->x + con->width)) {
            rect->width -= (rect->x + rect->width) - (con->x + con->width);
        }
        if ((rect->y + rect->height) > (con->y + con->height)) {
            rect->height -= (rect->y + rect->height) - (con->y + con->height);
        }
    }
}

gboolean
windowEvent(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    QTC_UNUSED(widget);
    if (GDK_FOCUS_CHANGE == event->type)
        gtk_widget_queue_draw((GtkWidget*)user_data);
    return FALSE;
}

void adjustToolbarButtons(GtkWidget *widget, int *x, int *y, int *width, int *height, int *round, gboolean horiz)
{
    GtkToolbar *toolbar = NULL;
    GtkToolItem *toolitem = NULL;
    GtkWidget *w = widget;

    for (int i = 0;i < 5 && w && (!toolbar || !toolitem);++i) {
        if (GTK_IS_TOOLBAR(w)) {
            toolbar = GTK_TOOLBAR(w);
        } else if (GTK_IS_TOOL_ITEM(w)) {
            toolitem = GTK_TOOL_ITEM(w);
        }
        w = gtk_widget_get_parent(w);
    }

    if(toolbar && toolitem)
    {
        int num=gtk_toolbar_get_n_items(toolbar);

        if(num>1)
        {
            int index=gtk_toolbar_get_item_index(toolbar, toolitem);
            GtkToolItem *prev=index ? gtk_toolbar_get_nth_item(toolbar, index-1) : NULL,
                        *next=index<(num-1) ? gtk_toolbar_get_nth_item(toolbar, index+1) : NULL;
            GtkWidget   *parent=NULL;
            gboolean    roundLeft=!prev || !GTK_IS_TOOL_BUTTON(prev),
                        roundRight=!next || !GTK_IS_TOOL_BUTTON(next),
                        isMenuButton=widget && GTK_IS_BUTTON(widget) &&
                                     (parent=gtk_widget_get_parent(widget)) && GTK_IS_BOX(parent) &&
                                     (parent=gtk_widget_get_parent(parent)) && GTK_IS_MENU_TOOL_BUTTON(parent),
                        isArrowButton=isMenuButton && GTK_IS_TOGGLE_BUTTON(widget);
            int         *pos=horiz ? x : y,
                        *size=horiz ? width : height;

            if(isArrowButton)
            {
                if(roundLeft && roundRight)
                    *round=horiz ? ROUNDED_RIGHT : ROUNDED_BOTTOM, *pos-=4, *size+=4;
                else if(roundLeft)
                    *round=ROUNDED_NONE, *pos-=4, *size+=8;
                else if(roundRight)
                    *round=horiz ? ROUNDED_RIGHT : ROUNDED_BOTTOM, *pos-=4, *size+=4;
                else
                    *round=ROUNDED_NONE, *pos-=4, *size+=8;
            }
            else if(isMenuButton)
            {
                if(roundLeft && roundRight)
                    *round=horiz ? ROUNDED_LEFT : ROUNDED_TOP, *size+=4;
                else if(roundLeft)
                    *round=horiz ? ROUNDED_LEFT : ROUNDED_TOP, *size+=4;
                else if(roundRight)
                    *round=ROUNDED_NONE, *pos-=4, *size+=8;
                else
                    *round=ROUNDED_NONE, *pos-=4, *size+=8;
            }
            else if(roundLeft && roundRight)
                ;
            else if(roundLeft)
                *round=horiz ? ROUNDED_LEFT : ROUNDED_TOP, *size+=4;
            else if(roundRight)
                *round=horiz ? ROUNDED_RIGHT : ROUNDED_BOTTOM, *pos-=4, *size+=4;
            else
                *round=ROUNDED_NONE, *pos-=4, *size+=8;
        }
    }
}

void getEntryParentBgCol(GtkWidget *widget, GdkColor *color)
{
    GtkWidget *parent=NULL;
    GtkStyle  *style=NULL;

    if (!widget)
    {
        color->red=color->green=color->blue = 65535;
        return;
    }

    parent = gtk_widget_get_parent(widget);

    while (parent && (!gtk_widget_get_has_window(parent)))
    {
        GtkStyle *style=NULL;
        if (opts.tabBgnd && GTK_IS_NOTEBOOK(parent) &&
            (style = gtk_widget_get_style(parent))) {
            qtcShade(&(style->bg[GTK_STATE_NORMAL]), color,
                      TO_FACTOR(opts.tabBgnd), opts.shading);
            return;
        }
        parent = gtk_widget_get_parent(parent);
    }

    if (!parent)
        parent = widget;

    style=gtk_widget_get_style(parent);

    if(style)
        *color = style->bg[gtk_widget_get_state(parent)];
}

gboolean compositingActive(GtkWidget *widget)
{
    GdkScreen *screen=widget ? gtk_widget_get_screen(widget) : gdk_screen_get_default();

    return screen && gdk_screen_is_composited(screen);
}

gboolean isRgbaWidget(GtkWidget *widget)
{
    if (widget) {
        GdkVisual *visual = gtk_widget_get_visual(widget);
#if GTK_CHECK_VERSION(2, 90, 0)
        guint32 redMask;
        guint32 greenMask;
        guint32 blueMask;

        gdk_visual_get_red_pixel_details(visual, &redMask, NULL, NULL);
        gdk_visual_get_green_pixel_details(visual, &greenMask, NULL, NULL);
        gdk_visual_get_blue_pixel_details(visual, &blueMask, NULL, NULL);

        return (32 == gdk_visual_get_depth(visual) && 0xff0000 == redMask &&
                0x00ff00 == greenMask && 0x0000ff == blueMask);
#else
        return (32 == visual->depth && 0xff0000 == visual->red_mask &&
                0x00ff00 == visual->green_mask &&
                0x0000ff == visual->blue_mask);
#endif
    }
    return FALSE;
}

#define BLUR_BEHIND_OBJECT "QTC_BLUR_BEHIND"
void
enableBlurBehind(GtkWidget *w, gboolean enable)
{
    GtkWindow *topLevel = GTK_WINDOW(gtk_widget_get_toplevel(w));
    if (topLevel) {
        int oldValue =
            GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),
                                              BLUR_BEHIND_OBJECT));

        if (oldValue == 0 || (enable && oldValue != 1) ||
            (!enable && oldValue != 2)) {
            g_object_set_data(G_OBJECT(w), QTC_MENUBAR_SIZE,
                              GINT_TO_POINTER(enable ? 1 : 2));
            xcb_window_t wid =
                GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(topLevel)));
            qtcX11BlurTrigger(wid, enable, 0, NULL);
        }
    }
}

void
getTopLevelSize(GdkWindow *window, int *w, int *h)
{
    if (!(window && GDK_IS_WINDOW(window))) {
        if (w) {
            *w = -1;
        }
        if (h) {
            *h = -1;
        }
    } else {
        GdkWindow *topLevel = gdk_window_get_toplevel(window);

        if (topLevel) {
            gdk_drawable_get_size(topLevel, w, h);
        } else {
            gdk_drawable_get_size(window, w, h);
        }
    }
}

// void getTopLevelFrameSize(GdkWindow *window, int *w, int *h)
// {
//     if(!(window && GDK_IS_WINDOW(window)))
//     {
//         if(w)
//             *w = -1;
//         if(h)
//             *h = -1;
//     }
//     else
//     {
//         GdkWindow *topLevel=gdk_window_get_toplevel(window);
//
//         if(topLevel)
//         {
//             GdkRectangle rect = {0, 0, -1, -1};
//
//             gdk_window_get_frame_extents(topLevel, &rect);
//             if(w)
//                 *w = rect.width;
//             if(h)
//                 *h = rect.height;
//         }
//     }
// }

void getTopLevelOrigin(GdkWindow *window, int *x, int *y)
{
    if(x)
        *x = 0;
    if(y)
        *y = 0;
    if(window)
    {
        while(window && GDK_IS_WINDOW(window) && gdk_window_get_window_type(window) != GDK_WINDOW_TOPLEVEL &&
              gdk_window_get_window_type(window) != GDK_WINDOW_TEMP)
        {
            int xloc;
            int yloc;
            gdk_window_get_position(window, &xloc, &yloc);
            if(x)
                *x += xloc;
            if(y)
                *y += yloc;
            window = gdk_window_get_parent(window);
        }
    }
}

gboolean mapToTopLevel(GdkWindow *window, GtkWidget *widget, int *x, int *y, int *w, int *h) //, gboolean frame)
{
    // always initialize arguments (to invalid values)
    if(x) *x=0;
    if(y) *y=0;
    if(w) *w = -1;
    if(h) *h = -1;

    if(!(window && GDK_IS_WINDOW(window)))
    {
        if(widget)
        {
            int xlocal, ylocal;

            // this is an alternative way to get widget position with respect to top level window
            // and top level window size. This is used in case the GdkWindow passed as argument is
            // actually a 'non window' drawable
            window = gtk_widget_get_parent_window(widget);
//             if(frame)
//                getTopLevelFrameSize(window, w, h);
//             else
                getTopLevelSize(window, w, h);

            if(gtk_widget_translate_coordinates(widget, gtk_widget_get_toplevel(widget), 0, 0, &xlocal, &ylocal))
            {

                if(x) *x=xlocal;
                if(y) *y=ylocal;
                return ((!w) || *w > 0) && ((!h) || *h>0);
            }
        }
    }
    else
    {
        // get window size and height
//         if(frame)
//            getTopLevelFrameSize(window, w, h);
//         else
            getTopLevelSize(window, w, h);
        getTopLevelOrigin( window, x, y );
        return ((!w) || *w > 0) && ((!h) || *h>0);
    }

    return FALSE;
}

gboolean treeViewCellHasChildren(GtkTreeView *treeView, GtkTreePath *path)
{
    // check treeview and path
    if(treeView && path)
    {
        GtkTreeModel *model=gtk_tree_view_get_model(treeView);
        if(model)
        {
            GtkTreeIter iter;
            if(gtk_tree_model_get_iter(model, &iter, path))
                return gtk_tree_model_iter_has_child(model, &iter);
        }
    }

    return FALSE;
}

gboolean treeViewCellIsLast(GtkTreeView *treeView, GtkTreePath *path)
{
    // check treeview and path
    if(treeView && path)
    {
        GtkTreeModel *model=gtk_tree_view_get_model(treeView);
        if(model)
        {
            GtkTreeIter iter;
            if(gtk_tree_model_get_iter(model, &iter, path))
                return !gtk_tree_model_iter_next(model, &iter);
        }
    }

    return FALSE;
}

GtkTreePath*
treeViewPathParent(GtkTreeView *treeView, GtkTreePath *path)
{
    QTC_UNUSED(treeView);
    if (path) {
        GtkTreePath *parent = gtk_tree_path_copy(path);
        if (gtk_tree_path_up(parent)) {
            return parent;
        } else {
            gtk_tree_path_free(parent);
        }
    }
    return NULL;
}

void generateColors()
{
    qtcShadeColors(&qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW], qtcPalette.background);
    qtcShadeColors(&qtSettings.colors[PAL_ACTIVE][COLOR_BUTTON], qtcPalette.button[PAL_ACTIVE]);
    qtcShadeColors(&qtSettings.colors[PAL_DISABLED][COLOR_BUTTON], qtcPalette.button[PAL_DISABLED]);
    qtcShadeColors(&qtSettings.colors[PAL_ACTIVE][COLOR_SELECTED], qtcPalette.highlight);
    qtcShadeColors(&qtSettings.colors[PAL_ACTIVE][COLOR_FOCUS], qtcPalette.focus);

    switch(opts.shadeMenubars)
    {
        case SHADE_WINDOW_BORDER:
            qtcPalette.wborder[0] = qtcNew(GdkColor, TOTAL_SHADES + 1);
            qtcPalette.wborder[1] = qtcNew(GdkColor, TOTAL_SHADES + 1);
            qtcShadeColors(&qtSettings.colors[PAL_INACTIVE][COLOR_WINDOW_BORDER], qtcPalette.wborder[0]);
            qtcShadeColors(&qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_BORDER], qtcPalette.wborder[1]);
            break;
        case SHADE_NONE:
            memcpy(qtcPalette.menubar, qtcPalette.background, sizeof(GdkColor)*(TOTAL_SHADES+1));
            break;
        case SHADE_BLEND_SELECTED:
        {
            GdkColor mid=midColor(&qtcPalette.highlight[ORIGINAL_SHADE], &qtcPalette.background[ORIGINAL_SHADE]);
            qtcShadeColors(&mid, qtcPalette.menubar);
            break;
        }
        case SHADE_SELECTED:
        {
            GdkColor color;

            if (IS_GLASS(opts.appearance)) {
                qtcShade(&qtcPalette.highlight[ORIGINAL_SHADE], &color,
                          MENUBAR_GLASS_SELECTED_DARK_FACTOR, opts.shading);
            } else {
                color = qtcPalette.highlight[ORIGINAL_SHADE];
            }

            qtcShadeColors(&color, qtcPalette.menubar);
            break;
        }
        case SHADE_CUSTOM:
            qtcShadeColors(&opts.customMenubarsColor, qtcPalette.menubar);
            break;
        case SHADE_DARKEN: {
            GdkColor color;
            qtcShade(&qtcPalette.background[ORIGINAL_SHADE], &color,
                      MENUBAR_DARK_FACTOR, opts.shading);
            qtcShadeColors(&color, qtcPalette.menubar);
            break;
        }
    }

    switch(opts.shadeSliders)
    {
        case SHADE_SELECTED:
            qtcPalette.slider=qtcPalette.highlight;
            break;
        case SHADE_CUSTOM:
            qtcPalette.slider = qtcNew(GdkColor, TOTAL_SHADES + 1);
            qtcShadeColors(&opts.customSlidersColor, qtcPalette.slider);
            break;
        case SHADE_BLEND_SELECTED:
        {
            GdkColor mid=midColor(&qtcPalette.highlight[ORIGINAL_SHADE],
                                  &qtcPalette.button[PAL_ACTIVE][ORIGINAL_SHADE]);

            qtcPalette.slider = qtcNew(GdkColor, TOTAL_SHADES + 1);
            qtcShadeColors(&mid, qtcPalette.slider);
        }
        default:
            break;
    }

    qtcPalette.combobtn=NULL;
    switch(opts.comboBtn)
    {
        case SHADE_SELECTED:
            qtcPalette.combobtn=qtcPalette.highlight;
            break;
        case SHADE_CUSTOM:
            if(SHADE_CUSTOM == opts.shadeSliders && EQUAL_COLOR(opts.customSlidersColor, opts.customComboBtnColor))
                qtcPalette.combobtn=qtcPalette.slider;
            else
            {
                qtcPalette.combobtn = qtcNew(GdkColor, TOTAL_SHADES + 1);
                qtcShadeColors(&opts.customComboBtnColor, qtcPalette.combobtn);
            }
            break;
        case SHADE_BLEND_SELECTED:
            if(SHADE_BLEND_SELECTED == opts.shadeSliders)
                qtcPalette.combobtn=qtcPalette.slider;
            else
            {
                GdkColor mid=midColor(&qtcPalette.highlight[ORIGINAL_SHADE],
                                      &qtcPalette.button[PAL_ACTIVE][ORIGINAL_SHADE]);

                qtcPalette.combobtn = qtcNew(GdkColor, TOTAL_SHADES + 1);
                qtcShadeColors(&mid, qtcPalette.combobtn);
            }
        default:
            break;
    }

    qtcPalette.sortedlv=NULL;
    switch(opts.sortedLv)
    {
        case SHADE_DARKEN:
        {
            GdkColor color;

            qtcPalette.sortedlv = qtcNew(GdkColor, TOTAL_SHADES + 1);
            qtcShade(opts.lvButton ?
                     &qtcPalette.button[PAL_ACTIVE][ORIGINAL_SHADE] :
                     &qtcPalette.background[ORIGINAL_SHADE],
                     &color, LV_HEADER_DARK_FACTOR, opts.shading);
            qtcShadeColors(&color, qtcPalette.sortedlv);
            break;
        }
        case SHADE_SELECTED:
            qtcPalette.sortedlv=qtcPalette.highlight;
            break;
        case SHADE_CUSTOM:
            if(SHADE_CUSTOM == opts.shadeSliders && EQUAL_COLOR(opts.customSlidersColor, opts.customSortedLvColor))
                qtcPalette.sortedlv=qtcPalette.slider;
            else if(SHADE_CUSTOM == opts.comboBtn && EQUAL_COLOR(opts.customComboBtnColor, opts.customSortedLvColor))
                qtcPalette.sortedlv=qtcPalette.combobtn;
            else
            {
                qtcPalette.sortedlv = qtcNew(GdkColor, TOTAL_SHADES + 1);
                qtcShadeColors(&opts.customSortedLvColor, qtcPalette.sortedlv);
            }
            break;
        case SHADE_BLEND_SELECTED:
            if(SHADE_BLEND_SELECTED == opts.shadeSliders)
                qtcPalette.sortedlv=qtcPalette.slider;
            else if(SHADE_BLEND_SELECTED == opts.comboBtn)
                qtcPalette.sortedlv=qtcPalette.combobtn;
            else
            {
                GdkColor mid=midColor(&qtcPalette.highlight[ORIGINAL_SHADE],
                                      opts.lvButton ? &qtcPalette.button[PAL_ACTIVE][ORIGINAL_SHADE]
                                                    : &qtcPalette.background[ORIGINAL_SHADE]);

                qtcPalette.sortedlv = qtcNew(GdkColor, TOTAL_SHADES + 1);
                qtcShadeColors(&mid, qtcPalette.sortedlv);
            }
        default:
            break;
    }

    switch(opts.defBtnIndicator)
    {
        case IND_TINT:
        {
            GdkColor col=tint(&qtcPalette.button[PAL_ACTIVE][ORIGINAL_SHADE],
                            &qtcPalette.highlight[ORIGINAL_SHADE], DEF_BNT_TINT);
            qtcPalette.defbtn = qtcNew(GdkColor, TOTAL_SHADES + 1);
            qtcShadeColors(&col, qtcPalette.defbtn);
            break;
        }
        case IND_GLOW:
        case IND_SELECTED:
            qtcPalette.defbtn=qtcPalette.highlight;
            break;
        default:
            break;
        case IND_COLORED:
            if(SHADE_BLEND_SELECTED == opts.shadeSliders)
                qtcPalette.defbtn=qtcPalette.slider;
            else
            {
                GdkColor mid=midColor(&qtcPalette.highlight[ORIGINAL_SHADE],
                                      &qtcPalette.button[PAL_ACTIVE][ORIGINAL_SHADE]);

                qtcPalette.defbtn = qtcNew(GdkColor, TOTAL_SHADES + 1);
                qtcShadeColors(&mid, qtcPalette.defbtn);
            }
    }

    if (opts.coloredMouseOver) {
        qtcPalette.mouseover = qtcNew(GdkColor, TOTAL_SHADES + 1);
        qtcShadeColors(&qtSettings.colors[PAL_ACTIVE][COLOR_HOVER],
                       qtcPalette.mouseover);
    }

    switch(opts.shadeCheckRadio)
    {
        default:
            qtcPalette.check_radio=&qtSettings.colors[PAL_ACTIVE][opts.crButton ? COLOR_BUTTON_TEXT : COLOR_TEXT];
            break;
        case SHADE_BLEND_SELECTED:
        case SHADE_SELECTED:
            qtcPalette.check_radio=&qtSettings.colors[PAL_ACTIVE][COLOR_SELECTED];
            break;
        case SHADE_CUSTOM:
            qtcPalette.check_radio=&opts.customCheckRadioColor;
    }

    {
        GdkColor color;
        GdkColor *cols=opts.shadePopupMenu
                            ? menuColors(TRUE)
                            : qtcPalette.background;
        if (opts.lighterPopupMenuBgnd) {
            qtcShade(&cols[ORIGINAL_SHADE], &color,
                      TO_FACTOR(opts.lighterPopupMenuBgnd), opts.shading);
        } else {
            color = cols[ORIGINAL_SHADE];
        }
        qtcShadeColors(&color, qtcPalette.menu);
    }

    /* Tear off menu items dont seem to draw they're background, and the default background
        is drawn :-(  Fix/hack this by making that background the correct color */
    if (opts.lighterPopupMenuBgnd || opts.shadePopupMenu) {
        static const char *format="style \""RC_SETTING"Mnu\" { "
                                    "bg[NORMAL]=\"#%02X%02X%02X\" "
                                    "fg[NORMAL]=\"#%02X%02X%02X\" "
                                    "text[INSENSITIVE]=\"#%02X%02X%02X\" "
                                    "} class \"GtkMenu\" style \""RC_SETTING"Mnu\" "
                                    "widget_class \"*Menu.*Label\" style \""RC_SETTING"Mnu\""
                                    " style  \""RC_SETTING"CView\" = \""RC_SETTING"Mnu\" { text[NORMAL]=\"#%02X%02X%02X\" } "
                                    " widget_class \"*<GtkMenuItem>*<GtkCellView>\" style \""RC_SETTING"CView\"";
        char *str = (char*)malloc(strlen(format) + 24 + 1);

        if (str) {
            GdkColor *col = &qtcPalette.menu[ORIGINAL_SHADE];
            GdkColor text=opts.shadePopupMenu
                            ? SHADE_WINDOW_BORDER == opts.shadeMenubars
                                ? qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_BORDER_TEXT]
                                : opts.customMenuTextColor
                                    ? opts.customMenuNormTextColor
                                    : SHADE_BLEND_SELECTED == opts.shadeMenubars || SHADE_SELECTED == opts.shadeMenubars ||
                                    (SHADE_CUSTOM == opts.shadeMenubars && TOO_DARK(qtcPalette.menubar[ORIGINAL_SHADE]))
                                    ? qtSettings.colors[PAL_ACTIVE][COLOR_TEXT_SELECTED]
                                    : qtSettings.colors[PAL_ACTIVE][COLOR_TEXT]
                            : qtSettings.colors[PAL_ACTIVE][COLOR_TEXT],
                     mid=opts.shadePopupMenu ? midColor(col, &text) : qtSettings.colors[PAL_DISABLED][COLOR_TEXT];
            sprintf(str, format, toQtColor(col->red), toQtColor(col->green), toQtColor(col->blue),
                                 toQtColor(text.red), toQtColor(text.green), toQtColor(text.blue),
                                 toQtColor(mid.red),  toQtColor(mid.green),  toQtColor(mid.blue),
                                 toQtColor(text.red), toQtColor(text.green), toQtColor(text.blue));
            gtk_rc_parse_string(str);
            free(str);
        }
    }

    switch(opts.menuStripe)
    {
        default:
        case SHADE_NONE:
            opts.customMenuStripeColor=qtcPalette.background[ORIGINAL_SHADE];
            break;
        case SHADE_DARKEN:
            opts.customMenuStripeColor =
                (opts.lighterPopupMenuBgnd || opts.shadePopupMenu ?
                 qtcPalette.menu[ORIGINAL_SHADE] :
                 qtcPalette.background[MENU_STRIPE_SHADE]);
            break;
        case SHADE_CUSTOM:
            break;
        case SHADE_BLEND_SELECTED:
            opts.customMenuStripeColor=midColor(&qtcPalette.highlight[ORIGINAL_SHADE],
                                                opts.lighterPopupMenuBgnd || opts.shadePopupMenu
                                                    ? &qtcPalette.menu[ORIGINAL_SHADE]
                                                    : &qtcPalette.background[ORIGINAL_SHADE]);
            break;
        case SHADE_SELECTED:
            opts.customMenuStripeColor=qtcPalette.highlight[MENU_STRIPE_SHADE];
    }

    qtcPalette.selectedcr=NULL;

    switch (opts.crColor) {
    case SHADE_DARKEN: {
        GdkColor color;
        qtcPalette.selectedcr = qtcNew(GdkColor, TOTAL_SHADES + 1);
        qtcShade(&qtcPalette.button[PAL_ACTIVE][ORIGINAL_SHADE], &color,
                 LV_HEADER_DARK_FACTOR, opts.shading);
        qtcShadeColors(&color, qtcPalette.selectedcr);
        break;
    }
        default:
        case SHADE_NONE:
            qtcPalette.selectedcr=qtcPalette.button[PAL_ACTIVE];
            break;
        case SHADE_SELECTED:
            qtcPalette.selectedcr=qtcPalette.highlight;
            break;
        case SHADE_CUSTOM:
            if(SHADE_CUSTOM == opts.shadeSliders && EQUAL_COLOR(opts.customSlidersColor, opts.customCrBgndColor))
                qtcPalette.selectedcr=qtcPalette.slider;
            else if(SHADE_CUSTOM == opts.comboBtn && EQUAL_COLOR(opts.customComboBtnColor, opts.customCrBgndColor))
                qtcPalette.selectedcr=qtcPalette.combobtn;
            else if(SHADE_CUSTOM == opts.sortedLv && EQUAL_COLOR(opts.customSortedLvColor, opts.customCrBgndColor))
                qtcPalette.selectedcr = qtcPalette.sortedlv;
            else {
                qtcPalette.selectedcr = qtcNew(GdkColor, TOTAL_SHADES + 1);
                qtcShadeColors(&opts.customCrBgndColor, qtcPalette.selectedcr);
            }
            break;
        case SHADE_BLEND_SELECTED:
            if(SHADE_BLEND_SELECTED == opts.shadeSliders)
                qtcPalette.selectedcr=qtcPalette.slider;
            else if(SHADE_BLEND_SELECTED == opts.comboBtn)
                qtcPalette.selectedcr=qtcPalette.combobtn;
            else if(SHADE_BLEND_SELECTED == opts.sortedLv)
                qtcPalette.selectedcr=qtcPalette.sortedlv;
            else
            {
                GdkColor mid =
                    midColor(&qtcPalette.highlight[ORIGINAL_SHADE],
                             &qtcPalette.button[PAL_ACTIVE][ORIGINAL_SHADE]);
                qtcPalette.selectedcr = qtcNew(GdkColor, TOTAL_SHADES + 1);
                qtcShadeColors(&mid, qtcPalette.selectedcr);
            }
    }

    qtcPalette.sidebar=NULL;
    if (!opts.stdSidebarButtons) {
        if (SHADE_BLEND_SELECTED == opts.shadeSliders) {
             qtcPalette.sidebar = qtcPalette.slider;
        } else if (IND_COLORED == opts.defBtnIndicator) {
            qtcPalette.sidebar = qtcPalette.defbtn;
        } else {
            GdkColor mid=midColor(&qtcPalette.highlight[ORIGINAL_SHADE], &qtcPalette.button[PAL_ACTIVE][ORIGINAL_SHADE]);

            qtcPalette.sidebar = qtcNew(GdkColor, TOTAL_SHADES + 1);
            qtcShadeColors(&mid, qtcPalette.sidebar);
        }
    }

    qtcPalette.progress=NULL;
    switch(opts.progressColor)
    {
        case SHADE_NONE:
            qtcPalette.progress=qtcPalette.background;
        default:
            /* Not set! */
            break;
        case SHADE_CUSTOM:
            if(SHADE_CUSTOM == opts.shadeSliders && EQUAL_COLOR(opts.customSlidersColor, opts.customProgressColor))
                qtcPalette.progress=qtcPalette.slider;
            else if(SHADE_CUSTOM == opts.comboBtn && EQUAL_COLOR(opts.customComboBtnColor, opts.customProgressColor))
                qtcPalette.progress=qtcPalette.combobtn;
            else if(SHADE_CUSTOM == opts.sortedLv && EQUAL_COLOR(opts.customSortedLvColor, opts.customProgressColor))
                qtcPalette.progress=qtcPalette.sortedlv;
            else if(SHADE_CUSTOM == opts.crColor && EQUAL_COLOR(opts.customCrBgndColor, opts.customProgressColor))
                qtcPalette.progress=qtcPalette.selectedcr;
            else {
                qtcPalette.progress = qtcNew(GdkColor, TOTAL_SHADES + 1);
                qtcShadeColors(&opts.customProgressColor, qtcPalette.progress);
            }
            break;
        case SHADE_BLEND_SELECTED:
            if(SHADE_BLEND_SELECTED == opts.shadeSliders)
                qtcPalette.progress=qtcPalette.slider;
            else if(SHADE_BLEND_SELECTED == opts.comboBtn)
                qtcPalette.progress=qtcPalette.combobtn;
            else if(SHADE_BLEND_SELECTED == opts.sortedLv)
                qtcPalette.progress=qtcPalette.sortedlv;
            else if(SHADE_BLEND_SELECTED == opts.crColor)
                qtcPalette.progress=qtcPalette.selectedcr;
            else
            {
                GdkColor mid=midColor(&qtcPalette.highlight[ORIGINAL_SHADE], &qtcPalette.background[ORIGINAL_SHADE]);

                qtcPalette.progress = qtcNew(GdkColor, TOTAL_SHADES + 1);
                qtcShadeColors(&mid, qtcPalette.progress);
            }
    }
}

GdkColor * getCheckRadioCol(GtkStyle *style, GtkStateType state, gboolean mnu)
{
    return !qtSettings.qt4 && mnu
                ? &style->text[state]
                : GTK_STATE_INSENSITIVE == state
                    ? &qtSettings.colors[PAL_DISABLED][opts.crButton ? COLOR_BUTTON_TEXT : COLOR_TEXT]
                    : qtcPalette.check_radio;
}
