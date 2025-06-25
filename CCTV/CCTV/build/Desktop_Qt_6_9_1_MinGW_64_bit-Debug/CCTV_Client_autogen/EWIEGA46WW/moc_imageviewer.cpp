/****************************************************************************
** Meta object code from reading C++ file 'imageviewer.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../imageviewer.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'imageviewer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN11ImageViewerE_t {};
} // unnamed namespace

template <> constexpr inline auto ImageViewer::qt_create_metaobjectdata<qt_meta_tag_ZN11ImageViewerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ImageViewer",
        "imageChanged",
        "",
        "index",
        "imageAdded",
        "totalCount",
        "zoomChanged",
        "factor",
        "onPreviousClicked",
        "onNextClicked",
        "onZoomInClicked",
        "onZoomOutClicked",
        "onResetZoomClicked",
        "onFitToWindowClicked",
        "onSaveClicked",
        "onZoomSliderChanged",
        "value"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'imageChanged'
        QtMocHelpers::SignalData<void(int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'imageAdded'
        QtMocHelpers::SignalData<void(int)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'zoomChanged'
        QtMocHelpers::SignalData<void(double)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 7 },
        }}),
        // Slot 'onPreviousClicked'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onNextClicked'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onZoomInClicked'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onZoomOutClicked'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onResetZoomClicked'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onFitToWindowClicked'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSaveClicked'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onZoomSliderChanged'
        QtMocHelpers::SlotData<void(int)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 16 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ImageViewer, qt_meta_tag_ZN11ImageViewerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ImageViewer::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11ImageViewerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11ImageViewerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN11ImageViewerE_t>.metaTypes,
    nullptr
} };

void ImageViewer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ImageViewer *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->imageChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->imageAdded((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->zoomChanged((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 3: _t->onPreviousClicked(); break;
        case 4: _t->onNextClicked(); break;
        case 5: _t->onZoomInClicked(); break;
        case 6: _t->onZoomOutClicked(); break;
        case 7: _t->onResetZoomClicked(); break;
        case 8: _t->onFitToWindowClicked(); break;
        case 9: _t->onSaveClicked(); break;
        case 10: _t->onZoomSliderChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ImageViewer::*)(int )>(_a, &ImageViewer::imageChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ImageViewer::*)(int )>(_a, &ImageViewer::imageAdded, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ImageViewer::*)(double )>(_a, &ImageViewer::zoomChanged, 2))
            return;
    }
}

const QMetaObject *ImageViewer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ImageViewer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11ImageViewerE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ImageViewer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void ImageViewer::imageChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ImageViewer::imageAdded(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void ImageViewer::zoomChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}
QT_WARNING_POP
