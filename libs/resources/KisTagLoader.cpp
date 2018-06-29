/*
 * Copyright (C) 2018 Boudewijn Rempt <boud@valdyas.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "KisTagLoader.h"

#include <QIODevice>

#include <kconfigini_p.h>

class KisTagLoader::Private {
};

KisTagLoader::KisTagLoader()
{
}

QString KisTagLoader::name() const
{
    return QString();
}

void KisTagLoader::setName(QString &name) const
{

}

QString KisTagLoader::url() const
{
    return QString();
}

void KisTagLoader::setUrl(const QString &url) const
{

}

QString KisTagLoader::comment() const
{
    return QString();
}

void KisTagLoader::setComment(const QString &comment) const
{

}

bool KisTagLoader::load(QIODevice &io)
{
    return false;
}

bool KisTagLoader::save(QIODevice &io)
{
    return false;
}
