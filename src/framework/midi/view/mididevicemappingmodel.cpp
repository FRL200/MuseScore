/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "mididevicemappingmodel.h"

#include "ui/view/iconcodes.h"
#include "shortcuts/shortcutstypes.h"

#include "log.h"
#include "translation.h"

using namespace mu::midi;
using namespace mu::ui;
using namespace mu::actions;
using namespace mu::shortcuts;

static const QString TITLE_KEY("title");
static const QString ICON_KEY("icon");
static const QString STATUS_KEY("status");
static const QString ENABLED_KEY("enabled");
static const QString MAPPED_VALUE_KEY("mappedValue");

inline std::vector<ActionCode> allMidiActions()
{
    return {
        "rewind",
        "loop",
        "play",
        "stop",
        "note-input",
        "pad-note-1",
        "pad-note-2",
        "pad-note-4",
        "pad-note-8",
        "pad-note-16",
        "pad-note-32",
        "pad-note-64",
        "undo",
        "pad-rest",
        "tie",
        "pad-dot",
        "pad-dotdot",
        "note-input-realtime-auto"
    };
}

MidiDeviceMappingModel::MidiDeviceMappingModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

QVariant MidiDeviceMappingModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    QVariantMap obj = midiMappingToObject(m_midiMappings[index.row()]);

    switch (role) {
    case RoleTitle: return obj[TITLE_KEY].toString();
    case RoleIcon: return obj[ICON_KEY].toInt();
    case RoleStatus: return obj[STATUS_KEY].toString();
    case RoleEnabled: return obj[ENABLED_KEY].toBool();
    case RoleMappedValue: return obj[MAPPED_VALUE_KEY].toInt();
    }

    return QVariant();
}

QVariantMap MidiDeviceMappingModel::midiMappingToObject(const MidiMapping& midiMapping) const
{
    UiAction action = uiActionsRegister()->action(midiMapping.action);

    QVariantMap obj;

    obj[TITLE_KEY] = action.title;
    obj[ICON_KEY] = static_cast<int>(action.iconCode);
    obj[STATUS_KEY] = midiMapping.isValid() ? "Active" : "Inactive";
    obj[MAPPED_VALUE_KEY] = midiMapping.isValid() ? midiMapping.event.to_MIDI10Package() : 0;

    return obj;
}

int MidiDeviceMappingModel::rowCount(const QModelIndex&) const
{
    return m_midiMappings.size();
}

QHash<int, QByteArray> MidiDeviceMappingModel::roleNames() const
{
    return {
        { RoleTitle, TITLE_KEY.toUtf8() },
        { RoleIcon, ICON_KEY.toUtf8() },
        { RoleStatus, STATUS_KEY.toUtf8() },
        { RoleEnabled, ENABLED_KEY.toUtf8() },
        { RoleMappedValue, MAPPED_VALUE_KEY.toUtf8() }
    };
}

void MidiDeviceMappingModel::load()
{
    beginResetModel();
    m_midiMappings.clear();

    shortcuts::MidiMappingList midiMappings = midiRemote()->midiMappings();

    auto midiEvent = [&midiMappings](const ActionCode& actionCode) {
        for (const MidiMapping& midiMapping : midiMappings) {
            if (midiMapping.action == actionCode) {
                return midiMapping.event;
            }
        }

        return Event();
    };

    for (const ActionCode& actionCode : allMidiActions()) {
        UiAction action = uiActionsRegister()->action(actionCode);

        if (action.isValid()) {
            MidiMapping midiMapping(actionCode);
            midiMapping.event = midiEvent(actionCode);
            m_midiMappings.push_back(midiMapping);
        }
    }

    endResetModel();
}

bool MidiDeviceMappingModel::apply()
{
    MidiMappingList midiMappings;
    for (const MidiMapping& midiMapping : m_midiMappings) {
        midiMappings.push_back(midiMapping);
    }

    Ret ret = midiRemote()->setMidiMappings(midiMappings);
    if (!ret) {
        LOGE() << ret.toString();
    }

    return ret;
}

bool MidiDeviceMappingModel::useRemoteControl() const
{
    return configuration()->useRemoteControl();
}

void MidiDeviceMappingModel::setUseRemoteControl(bool value)
{
    if (value == useRemoteControl()) {
        return;
    }

    configuration()->setUseRemoteControl(value);
    emit useRemoteControlChanged(value);
}

QItemSelection MidiDeviceMappingModel::selection() const
{
    return m_selection;
}

bool MidiDeviceMappingModel::canEditAction() const
{
    return currentAction().isValid();
}

void MidiDeviceMappingModel::setSelection(const QItemSelection& selection)
{
    if (selection == m_selection) {
        return;
    }

    m_selection = selection;
    emit selectionChanged(selection);
}

void MidiDeviceMappingModel::clearSelectedActions()
{
    NOT_IMPLEMENTED;
}

void MidiDeviceMappingModel::clearAllActions()
{
    NOT_IMPLEMENTED;
}

QVariant MidiDeviceMappingModel::currentAction() const
{
    if (m_selection.size() != 1) {
        return QVariant();
    }

    MidiMapping midiMapping = m_midiMappings[m_selection.indexes().first().row()];
    return midiMappingToObject(midiMapping);
}

void MidiDeviceMappingModel::mapCurrentActionToMidiValue(int value)
{
    m_midiMappings[m_selection.indexes().first().row()].event = Event::fromMIDI10Package(value);
    emit dataChanged(m_selection.indexes().first(), m_selection.indexes().first());
}
