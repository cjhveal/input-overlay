/*************************************************************************
 * This file is part of input-overlay
 * github.con/univrsal/input-overlay
 * Copyright 2020 univrsal <universailp@web.de>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/

#include "gamepad_binding.hpp"
#include "util/settings.h"
#include "util/log.h"
#include "util/obs_util.hpp"
#include <keycodes.h>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <obs-module.h>

namespace gamepad {
std::vector<std::shared_ptr<bindings>> loaded_bindings;

std::vector<bind> bindings::m_defaults = {{S_BINDING_A, "txt_a", VC_PAD_A, false},
										  {S_BINDING_B, "txt_b", VC_PAD_B, false},
										  {S_BINDING_X, "txt_x", VC_PAD_X, false},
										  {S_BINDING_Y, "txt_y", VC_PAD_Y, false},
										  {S_BINDING_GUIDE, "txt_guide", VC_PAD_GUIDE, false},
										  {S_BINDING_LT, "txt_lt", VC_PAD_LT, true},
										  {S_BINDING_RT, "txt_rt", VC_PAD_RT, true},
										  {S_BINDING_RB, "txt_rb", VC_PAD_RB, false},
										  {S_BINDING_START, "txt_start", VC_PAD_START, false},
										  {S_BINDING_BACK, "txt_back", VC_PAD_BACK, false},
										  {S_BINDING_DPAD_UP, "txt_dpad_up", VC_PAD_DPAD_UP, false},
										  {S_BINDING_DPAD_DOWN, "txt_dpad_down", VC_PAD_DPAD_DOWN, false},
										  {S_BINDING_DPAD_LEFT, "txt_dpad_left", VC_PAD_DPAD_LEFT, false},
										  {S_BINDING_LB, "txt_lb", VC_PAD_LB, false},
										  {S_BINDING_DPAD_RIGHT, "txt_dpad_right", VC_PAD_DPAD_RIGHT, false},
										  {S_BINDING_ANALOG_L, "txt_analog_left", VC_PAD_L_ANALOG, false},
										  {S_BINDING_ANALOG_R, "txt_analog_right", VC_PAD_R_ANALOG, false},
										  {S_BINDING_ANALOG_LX, "txt_lx", VC_PAD_LX, true},
										  {S_BINDING_ANALOG_LY, "txt_ly", VC_PAD_LX, true},
										  {S_BINDING_ANALOG_RX, "txt_rx", VC_PAD_RX, true},
										  {S_BINDING_ANALOG_RY, "txt_ry", VC_PAD_RY, true}};

bindings::bindings()
{
	/* Copy over defaults */
	for (const auto &b : m_defaults) {
		m_bindings[b.code] = b;
	}
}

bindings::bindings(const QJsonObject &obj)
{
	for (const auto &b : m_defaults) {
		m_bindings[b.code] = b;
	}

	const auto &binds = obj["binds"].toArray();
	const auto &devs = obj["devices"].toArray();

	m_name = obj["name"].toString();

	for (const auto &b : binds) {
		const auto &o = b.toObject();
		uint16_t in = o["in"].toInt();
		m_bindings[in].code = o["out"].toInt();
		m_bindings[in].axis_event = o["is_axis"].toBool();
	}

	for (const auto &d : devs) {
		m_bound_devices.append(d.toString());
	}
}

uint16_t bindings::translate(uint16_t in)
{
	return m_bindings[in].code;
}

void bindings::write_to_json(QJsonObject &obj) const
{
	QJsonArray binds, devs;
	for (const auto &b : m_bindings) {
		QJsonObject obj;
		obj["in"] = b.first;
		obj["out"] = b.second.code;
		obj["is_axis"] = b.second.axis_event;
		binds.append(obj);
	}

	for (const auto &dev : m_bound_devices)
		devs.append(dev);
	obj["binds"] = binds;
	obj["devices"] = devs;
	obj["name"] = m_name;
}

const QStringList &bindings::get_devices() const
{
	return m_bound_devices;
}

std::shared_ptr<bindings> get_binding_for_device(const QString &id)
{
	for (auto &b : loaded_bindings) {
		if (b->get_devices().contains(id))
			return b;
	}
	return nullptr;
}

void load_bindings()
{
	QFile f(QDir::home().filePath(".config/gamepad_bindings.json"));
	QJsonParseError err;

	if (!f.open(QIODevice::ReadOnly)) {
		bwarn("Couldn't load gamepad bindings");
		return;
	}

	const auto doc = QJsonDocument::fromJson(f.readAll(), &err);

	if (err.error == QJsonParseError::NoError && doc.isArray()) {
		auto bindings = doc.array();
		for (auto e : bindings) {
			if (!e.isObject())
				continue;
			loaded_bindings.emplace_back(e.toObject());
		}
	} else {
		bwarn("Json parse error: %s", qt_to_utf8(err.errorString()));
	}
}

void save_bindings()
{
	QString path = QDir::home().filePath(".config/gamepad_bindings.json");
	QFile f(path);
	QJsonDocument d;

	if (!f.open(QIODevice::WriteOnly)) {
		bwarn("Couldn't save gamepad bindings");
		return;
	}

	QJsonArray bindings;

	for (const auto &b : loaded_bindings) {
		QJsonObject obj;
		b->write_to_json(obj);
		bindings.append(obj);
	}

	d.setArray(bindings);
	auto data = d.toJson();
	auto wrote = f.write(data);

	if (data.length() != wrote) {
		berr("Couldn't write gamepad bindings to %s", qt_to_utf8(path));
	}
	f.close();
}

}
