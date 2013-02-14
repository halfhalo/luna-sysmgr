/* @@@LICENSE
*
*      Copyright (c) 2013 Hewlett-Packard Development Company, L.P.
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
#ifndef WEBOSTAPGESTURERECOGNIZER_H
#define WEBOSTAPGESTURERECOGNIZER_H

#include <QGestureRecognizer>

class WebosTapGestureRecognizer : public QGestureRecognizer
{
public:

    WebosTapGestureRecognizer() { }
    virtual ~WebosTapGestureRecognizer() { }

    virtual QGesture* create (QObject* target);
    virtual QGestureRecognizer::Result recognize (QGesture* gesture, QObject* watched, QEvent* event);
    virtual void reset (QGesture* gesture);

};
#endif // WEBOSTAPGESTURERECOGNIZER_H
