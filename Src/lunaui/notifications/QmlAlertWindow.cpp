/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */




#include "Common.h"

#include <qglobal.h>

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QDeclarativeComponent>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#else
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#endif
#include <QUrl>

#include "QmlAlertWindow.h"
#include "Settings.h"
#include "WindowServerLuna.h"

QmlAlertWindow::QmlAlertWindow(const QString& path, int width, int height)
	: AlertWindow(Window::Type_PopupAlert, width, height, true)
{
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
	QDeclarativeEngine* qmlEngine = WindowServer::instance()->declarativeEngine();
#else
    QQmlEngine* qmlEngine = WindowServer::instance()->qmlEngine();
#endif
	if (qmlEngine) {
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
		QDeclarativeContext* context =	qmlEngine->rootContext();
#else
        QQmlContext* context =	qmlEngine->rootContext();
#endif
		Settings* settings = Settings::LunaSettings();
		QUrl url = QUrl::fromLocalFile(path);
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
		m_qmlComp = new QDeclarativeComponent(qmlEngine, url, this);
#else
        m_qmlComp = new QQmlComponent(qmlEngine, url, this);
#endif
        if (m_qmlComp) {
			m_gfxObj = qobject_cast<QGraphicsObject*>(m_qmlComp->create());
			if (m_gfxObj) {
				m_gfxObj->setPos(-width/2, -height/2);
				m_gfxObj->setParentItem(this);

				connect(m_gfxObj, SIGNAL(okButtonPressed()), SLOT(slotClose()), Qt::QueuedConnection);
			}
		}
	}
}

QmlAlertWindow::~QmlAlertWindow()
{
}

void QmlAlertWindow::resizeEventSync(int w, int h)
{
    // NO-OP
}

void QmlAlertWindow::close()
{
	g_debug("%s", __PRETTY_FUNCTION__);
	WindowServer::instance()->removeWindow(this);
}

void QmlAlertWindow::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{

}

void QmlAlertWindow::slotClose()
{
    close();
}

void QmlAlertWindow::slotMore()
{
    //open up another window?
    close();
}
