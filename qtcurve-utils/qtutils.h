/*****************************************************************************
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

#ifndef __QTC_UTILS_QT_UTILS_H__
#define __QTC_UTILS_QT_UTILS_H__

#include "utils.h"
#include <QtGlobal>
#include <QWidget>
#include <config.h>

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0) && defined QTC_QT4_ENABLE_KDE) || \
    (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) && defined QTC_QT5_ENABLE_KDE)
#define _QTC_QT_ENABLE_KDE 1
#else
#define _QTC_QT_ENABLE_KDE 0
#endif

#define qtcCheckKDEType(obj, type) (qtcCheckKDETypeFull(obj, type, #type))
#define qtcCheckKDEType0(obj, type) (qtcCheckKDETypeFull0(obj, type, #type))

template <class T, class T2> static inline bool
qtcCheckType0(T2 *obj)
{
    return obj && qobject_cast<const T*>(obj);
}
template <class T2> static inline bool
qtcCheckType0(T2 *obj, const char *name)
{
    return obj && obj->inherits(name);
}

#if _QTC_QT_ENABLE_KDE
#define qtcCheckKDETypeFull(obj, type, name) (qobject_cast<const type*>(obj))
#define qtcCheckKDETypeFull0(obj, type, name) (qtcCheckType0<type>(obj))
#else
#define qtcCheckKDETypeFull(obj, type, name) (obj->inherits(name))
#define qtcCheckKDETypeFull0(obj, type, name) (qtcCheckType0(obj, name))
#endif

QTC_ALWAYS_INLINE static inline WId
qtcGetWid(const QWidget *w)
{
    if (!(w && w->testAttribute(Qt::WA_WState_Created)))
        return (WId)0;
    return w->internalWinId();
}

QTC_ALWAYS_INLINE static inline Qt::WindowType
qtcGetWindowType(const QWidget *w)
{
    return w ? w->windowType() : (Qt::WindowType)0;
}

QTC_ALWAYS_INLINE static inline bool
qtcIsDialog(const QWidget *w)
{
    return qtcOneOf(qtcGetWindowType(w), Qt::Dialog, Qt::Sheet);
}

QTC_ALWAYS_INLINE static inline bool
qtcIsWindow(const QWidget *w)
{
    return qtcOneOf(qtcGetWindowType(w), Qt::Window);
}

QTC_ALWAYS_INLINE static inline bool
qtcIsToolTip(const QWidget *w)
{
    return qtcOneOf(qtcGetWindowType(w), Qt::Tool, Qt::SplashScreen,
                    Qt::ToolTip, Qt::Drawer);
}

QTC_ALWAYS_INLINE static inline bool
qtcIsPopup(const QWidget *w)
{
    return qtcOneOf(qtcGetWindowType(w), Qt::Popup);
}

QTC_ALWAYS_INLINE static inline QWidget*
qtcToWidget(QObject *obj)
{
    if (obj->isWidgetType())
        return static_cast<QWidget*>(obj);
    return NULL;
}

QTC_ALWAYS_INLINE static inline const QWidget*
qtcToWidget(const QObject *obj)
{
    if (obj->isWidgetType())
        return static_cast<const QWidget*>(obj);
    return NULL;
}

template<class StyleType, class C>
static inline StyleType*
_qtcGetStyle(const C *obj)
{
    QStyle *style = obj->style();
    return style ? qobject_cast<StyleType*>(style) : 0;
}
#define qtcGetStyle(obj) _qtcGetStyle<Style>(obj)

// Get the width and direction of the arrow in a QBalloonTip
QTC_ALWAYS_INLINE static inline int
qtcGetBalloonMargin(QWidget *widget, bool *atTop)
{
    int topMargin = 0;
    int bottomMargin = 0;
    widget->getContentsMargins(NULL, &topMargin, NULL, &bottomMargin);
    if (topMargin > bottomMargin) {
        *atTop = true;
        return topMargin - bottomMargin;
    } else {
        *atTop = false;
        return bottomMargin - topMargin;
    }
}

#endif
