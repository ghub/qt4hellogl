/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <cassert>
#include <math.h>

#include <QtGui>
#include <QtOpenGL>

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include <osg/Camera>
#include <osgDB/ReadFile>
#include <osg/Geode>
#include <osg/Material>
#include <osg/NodeVisitor>
#include <osg/ShapeDrawable>
#include <osgViewer/View>

#include "glwidget.h"
#include "qtlogo.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

class EnableVboVisitor : public osg::NodeVisitor
{
public:
    EnableVboVisitor()
        : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}

    void apply(osg::Geode& geode)
    {
        for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
        {
            osg::Geometry* geometry = geode.getDrawable(i)->asGeometry();
            if (geometry)
            {
                geometry->setUseVertexBufferObjects(true);
            }
        }
    }
};

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(
        QGLFormat(QGL::SampleBuffers),
        parent)
    , logo(0)
    , xRot(0)
    , yRot(0)
    , zRot(0)
    , qtGreen(QColor::fromCmykF(
                  0.40,
                  0.0,
                  1.0,
                  0.0))
    , qtPurple(QColor::fromCmykF(
                   0.39,
                   0.39,
                   0.0,
                   0.0))
    , graphicsWindow_(new osgViewer::GraphicsWindowEmbedded(
                          this->x(),
                          this->y(),
                          this->width(),
                          this->height()))
    , camera_(new osg::Camera)
    , view_(new osgViewer::View)
    , viewer_(new osgViewer::CompositeViewer)
{
}

GLWidget::~GLWidget()
{
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize GLWidget::sizeHint() const
{
    return QSize(400, 400);
}

static void qNormalizeAngle(int &angle)
{
    while (angle < 0)
    {
        angle += 360 * 16;
    }
    while (angle > 360 * 16)
    {
        angle -= 360 * 16;
    }
}

void GLWidget::setXRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != xRot)
    {
        xRot = angle;
        emit xRotationChanged(angle);
        updateGL();
    }
}

void GLWidget::setYRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != yRot)
    {
        yRot = angle;
        emit yRotationChanged(angle);
        updateGL();
    }
}

void GLWidget::setZRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != zRot)
    {
        zRot = angle;
        emit zRotationChanged(angle);
        updateGL();
    }
}

void GLWidget::initializeGL()
{
    initializeQt();
    initializeOsg();
}

void GLWidget::initializeQt()
{
    static GLfloat lightPosition[4] = { 0.5, 5.0, 7.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    logo = new QtLogo(this, 64);
    logo->setColor(qtGreen.dark());

    // This ensures that the widget will receive keyboard events. This focus
    // policy is not set by default. The default, Qt::NoFocus, will result in
    // keyboard events that are ignored.
    this->setFocusPolicy(Qt::StrongFocus);
    this->setMinimumSize(100, 100);

    // Ensures that the widget receives mouse move events even though no
    // mouse button has been pressed. We require this in order to let the
    // graphics window switch viewports properly.
    this->setMouseTracking(true);
}

void GLWidget::initializeOsg()
{
    camera_->setClearMask(0);
    camera_->setGraphicsContext(graphicsWindow_);

    view_->setCamera(camera_);
    view_->setSceneData(createOsgModel());

    viewer_->addView(view_);
    viewer_->setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);
}

osg::Node* GLWidget::createOsgModel()
{
#if 0
    osg::ShapeDrawable* shape = new osg::ShapeDrawable(
        new osg::Box(
            osg::Vec3(0.f, 0.f, 0.f),
            1.0f));
    shape->setColor(osg::Vec4(1.f, 0.f, 0.f, 0.5f));

    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(shape);

    osg::Group* root = new osg::Group;
    root->addChild(geode);

    // Set material for basic lighting and enable depth tests. Else, the box
    // will suffer from rendering errors.
    {
        osg::StateSet* stateSet = root->getOrCreateStateSet();
        osg::Material* material = new osg::Material;

        material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);

        stateSet->setAttributeAndModes(material, osg::StateAttribute::ON);
        stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    }
#else
    osg::Node* root = osgDB::readNodeFile("cow.osg");
#endif

    EnableVboVisitor vboEnabler;
    root->accept(vboEnabler);

    return root;
}

void GLWidget::paintGL()
{
    paintQt();
    paintOsg();
}

void GLWidget::paintQt()
{
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glEnable(GL_MULTISAMPLE);
    glShadeModel(GL_SMOOTH);
    qglClearColor(qtPurple.dark());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(30.0f, aspectRatio(), 1.0f, 1000.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -10.0);
    glRotatef(xRot / 16.0, 1.0, 0.0, 0.0);
    glRotatef(yRot / 16.0, 0.0, 1.0, 0.0);
    glRotatef(zRot / 16.0, 0.0, 0.0, 1.0);

    logo->draw();
}

void GLWidget::paintOsg()
{
    GLdouble glMat[16];

    glGetDoublev(GL_MODELVIEW_MATRIX, glMat);
    camera_->setViewMatrix(osg::Matrix(glMat));

    glGetDoublev(GL_PROJECTION_MATRIX, glMat);
    camera_->setProjectionMatrix(osg::Matrix(glMat));

    viewer_->frame();
}

void GLWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);

    graphicsWindow_->resized(this->x(), this->y(), width, height);

    this->onResize(width, height);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastPos.x();
    int dy = event->y() - lastPos.y();

    if (event->buttons() & Qt::LeftButton)
    {
        setXRotation(xRot + 8 * dy);
        setYRotation(yRot + 8 * dx);
    }
    else if (event->buttons() & Qt::RightButton)
    {
        setXRotation(xRot + 8 * dy);
        setZRotation(zRot + 8 * dx);
    }
    lastPos = event->pos();
}

void GLWidget::onResize(int width, int height)
{
    camera_->setViewport(0, 0, this->width(), this->height());
}

float GLWidget::aspectRatio() const
{
    return static_cast<float>(this->width()) / static_cast<float>(this->height());
}

