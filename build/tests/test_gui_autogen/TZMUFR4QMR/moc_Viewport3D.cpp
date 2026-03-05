/****************************************************************************
** Meta object code from reading C++ file 'Viewport3D.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/rendering/Viewport3D.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Viewport3D.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.2. It"
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
struct qt_meta_tag_ZN3prs10Viewport3DE_t {};
} // unnamed namespace

template <> constexpr inline auto prs::Viewport3D::qt_create_metaobjectdata<qt_meta_tag_ZN3prs10Viewport3DE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "prs::Viewport3D",
        "modelLoaded",
        "",
        "filepath",
        "pointPlaced",
        "index",
        "pointSelected",
        "pointDeselected",
        "surfaceSelected",
        "surfaceDeselected",
        "surfaceMaterialChanged",
        "materialName",
        "placementModeChanged",
        "enabled",
        "scaleChanged",
        "factor",
        "measurementResult",
        "distance",
        "moveFinished",
        "pointIndex",
        "PlacedPoint",
        "oldState",
        "newState"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'modelLoaded'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'pointPlaced'
        QtMocHelpers::SignalData<void(int)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'pointSelected'
        QtMocHelpers::SignalData<void(int)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'pointDeselected'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'surfaceSelected'
        QtMocHelpers::SignalData<void(int)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'surfaceDeselected'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'surfaceMaterialChanged'
        QtMocHelpers::SignalData<void(int, const QString &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 }, { QMetaType::QString, 11 },
        }}),
        // Signal 'placementModeChanged'
        QtMocHelpers::SignalData<void(bool)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 13 },
        }}),
        // Signal 'scaleChanged'
        QtMocHelpers::SignalData<void(float)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Float, 15 },
        }}),
        // Signal 'measurementResult'
        QtMocHelpers::SignalData<void(float)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Float, 17 },
        }}),
        // Signal 'moveFinished'
        QtMocHelpers::SignalData<void(int, const PlacedPoint &, const PlacedPoint &)>(18, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 19 }, { 0x80000000 | 20, 21 }, { 0x80000000 | 20, 22 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Viewport3D, qt_meta_tag_ZN3prs10Viewport3DE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject prs::Viewport3D::staticMetaObject = { {
    QMetaObject::SuperData::link<QOpenGLWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN3prs10Viewport3DE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN3prs10Viewport3DE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN3prs10Viewport3DE_t>.metaTypes,
    nullptr
} };

void prs::Viewport3D::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Viewport3D *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->modelLoaded((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->pointPlaced((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->pointSelected((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->pointDeselected(); break;
        case 4: _t->surfaceSelected((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->surfaceDeselected(); break;
        case 6: _t->surfaceMaterialChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 7: _t->placementModeChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 8: _t->scaleChanged((*reinterpret_cast<std::add_pointer_t<float>>(_a[1]))); break;
        case 9: _t->measurementResult((*reinterpret_cast<std::add_pointer_t<float>>(_a[1]))); break;
        case 10: _t->moveFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<PlacedPoint>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<PlacedPoint>>(_a[3]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(const QString & )>(_a, &Viewport3D::modelLoaded, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(int )>(_a, &Viewport3D::pointPlaced, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(int )>(_a, &Viewport3D::pointSelected, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)()>(_a, &Viewport3D::pointDeselected, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(int )>(_a, &Viewport3D::surfaceSelected, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)()>(_a, &Viewport3D::surfaceDeselected, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(int , const QString & )>(_a, &Viewport3D::surfaceMaterialChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(bool )>(_a, &Viewport3D::placementModeChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(float )>(_a, &Viewport3D::scaleChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(float )>(_a, &Viewport3D::measurementResult, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(int , const PlacedPoint & , const PlacedPoint & )>(_a, &Viewport3D::moveFinished, 10))
            return;
    }
}

const QMetaObject *prs::Viewport3D::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *prs::Viewport3D::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN3prs10Viewport3DE_t>.strings))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QOpenGLFunctions"))
        return static_cast< QOpenGLFunctions*>(this);
    return QOpenGLWidget::qt_metacast(_clname);
}

int prs::Viewport3D::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QOpenGLWidget::qt_metacall(_c, _id, _a);
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
void prs::Viewport3D::modelLoaded(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void prs::Viewport3D::pointPlaced(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void prs::Viewport3D::pointSelected(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void prs::Viewport3D::pointDeselected()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void prs::Viewport3D::surfaceSelected(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void prs::Viewport3D::surfaceDeselected()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void prs::Viewport3D::surfaceMaterialChanged(int _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2);
}

// SIGNAL 7
void prs::Viewport3D::placementModeChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void prs::Viewport3D::scaleChanged(float _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}

// SIGNAL 9
void prs::Viewport3D::measurementResult(float _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}

// SIGNAL 10
void prs::Viewport3D::moveFinished(int _t1, const PlacedPoint & _t2, const PlacedPoint & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1, _t2, _t3);
}
QT_WARNING_POP
