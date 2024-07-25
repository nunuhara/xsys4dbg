/* Copyright (C) 2023 Nunuhara Cabbage <nunuhara@haniwa.technology>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QImage>
#include <QPixmap>

#include "xsystem4.hpp"

Color::Color(const QJsonValue &val)
{
	if (val.isObject()) {
		QJsonObject obj = val.toObject();
		r = obj["r"].toInt();
		g = obj["g"].toInt();
		b = obj["b"].toInt();
		a = obj["a"].toInt();
	} else if (val.isArray()) {
		QJsonArray arr = val.toArray();
		r = arr[0].toInt();
		g = arr[1].toInt();
		b = arr[2].toInt();
		a = arr[3].toInt();
	} else {
		qDebug() << "invalid Color object" << val;
		r = -1;
		g = -1;
		b = -1;
		a = -1;
	}
}

QString Color::toString() const
{
	return QString("(%1 %2 %3 %4)").arg(r).arg(g).arg(b).arg(a);
}

Rectangle::Rectangle(const QJsonValue &val)
{
	if (val.isObject()) {
		QJsonObject obj = val.toObject();
		x = obj["x"].toInt();
		y = obj["y"].toInt();
		w = obj["w"].toInt();
		h = obj["h"].toInt();
	} else if (val.isArray()) {
		QJsonArray arr = val.toArray();
		x = arr[0].toInt();
		y = arr[1].toInt();
		w = arr[2].toInt();
		h = arr[3].toInt();
	} else {
		qDebug() << "invalid Rectangle object" << val;
		x = -1;
		y = -1;
		w = -1;
		h = -1;
	}
}

QString Rectangle::toString() const
{
	return QString("(%1 %2 %3 %4)").arg(x).arg(y).arg(w).arg(h);
}

Point::Point(const QJsonValue &val)
{
	if (val.isObject()) {
		QJsonObject obj = val.toObject();
		x = obj["x"].toInt();
		y = obj["y"].toInt();
	} else if (val.isArray()) {
		QJsonArray arr = val.toArray();
		x = arr[0].toInt();
		y = arr[1].toInt();
	} else {
		qDebug() << "invalid Point object" << val;
		x = -1;
		y = -1;
	}
}

QString Point::toString() const
{
	return QString("(%1 %2)").arg(x).arg(y);
}

Point3D::Point3D(const QJsonValue &val)
{
	if (val.isObject()) {
		QJsonObject obj = val.toObject();
		x = obj["x"].toInt();
		y = obj["y"].toInt();
		z = obj["z"].toInt();
	} else if (val.isArray()) {
		QJsonArray arr = val.toArray();
		x = arr[0].toInt();
		y = arr[1].toInt();
		z = arr[2].toInt();
	} else {
		qDebug() << "invalid Point3D object:" << val;
		x = -1;
		y = -1;
		z = -1;
	}
}

QString Point3D::toString() const
{
	return QString("(%1 %2 %3)").arg(x).arg(y).arg(z);
}

TextStyle::TextStyle(const QJsonValue &val)
{
	if (!val.isObject()) {
		qDebug() << "invalid TextStyle object:" << val;
		return;
	}
}

static PartsType stringToPartsType(const QString &name)
{
	if (name == "uninitialized")
		return PARTS_UNINITIALIZED;
	if (name == "cg")
		return PARTS_CG;
	if (name == "text")
		return PARTS_TEXT;
	if (name == "animation")
		return PARTS_ANIMATION;
	if (name == "numeral")
		return PARTS_NUMERAL;
	if (name == "hgauge")
		return PARTS_HGAUGE;
	if (name == "vgauge")
		return PARTS_VGAUGE;
	if (name == "construction_process")
		return PARTS_CONSTRUCTION_PROCESS;
	return PARTS_INVALID;
}

static PartsCpType stringToPartsCpType(const QString &name)
{
	if (name == "create")
		return PARTS_CP_CREATE;
	if (name == "create_pixel_only")
		return PARTS_CP_CREATE_PIXEL_ONLY;
	if (name == "cg")
		return PARTS_CP_CG;
	if (name == "fill")
		return PARTS_CP_FILL;
	if (name == "fill_alpha_color")
		return PARTS_CP_FILL_ALPHA_COLOR;
	if (name == "fill_amap")
		return PARTS_CP_FILL_AMAP;
	if (name == "draw_cut_cg")
		return PARTS_CP_DRAW_CUT_CG;
	if (name == "copy_cut_cg")
		return PARTS_CP_COPY_CUT_CG;
	if (name == "draw_text")
		return PARTS_CP_DRAW_TEXT;
	if (name == "copy_text")
		return PARTS_CP_COPY_TEXT;
	return PARTS_CP_INVALID;
}

PartsCpOp::PartsCpOpData::PartsCpOpData(PartsCpType type)
{
	switch (type) {
	case PARTS_CP_CREATE:
	case PARTS_CP_CREATE_PIXEL_ONLY:
		new(&create) PartsCpCreate();
		break;
	case PARTS_CP_CG:
		new(&cg) PartsCpCg();
		break;
	case PARTS_CP_FILL:
	case PARTS_CP_FILL_ALPHA_COLOR:
	case PARTS_CP_FILL_AMAP:
		new(&fill) PartsCpFill();
		break;
	case PARTS_CP_DRAW_CUT_CG:
	case PARTS_CP_COPY_CUT_CG:
		new(&cutCg) PartsCpCutCg();
		break;
	case PARTS_CP_DRAW_TEXT:
	case PARTS_CP_COPY_TEXT:
		new(&text) PartsCpText();
		break;
	case PARTS_CP_INVALID:
		break;
	}
}

PartsCpOp::PartsCpOp(const QJsonObject &obj)
	: type(stringToPartsCpType(obj["type"].toString()))
	, data(type)
{
	switch (type) {
	case PARTS_CP_CREATE:
	case PARTS_CP_CREATE_PIXEL_ONLY:
		data.create.width = obj["size"].toObject()["w"].toInt();
		data.create.height = obj["size"].toObject()["h"].toInt();
		break;
	case PARTS_CP_CG:
		data.cg.no = obj["no"].toInt();
		break;
	case PARTS_CP_FILL:
	case PARTS_CP_FILL_ALPHA_COLOR:
	case PARTS_CP_FILL_AMAP:
		data.fill.rect = Rectangle(obj["rect"]);
		data.fill.color = Color(obj["color"]);
		break;
	case PARTS_CP_DRAW_CUT_CG:
	case PARTS_CP_COPY_CUT_CG:
		data.cutCg.dst = Rectangle(obj["dst"]);
		data.cutCg.src = Rectangle(obj["src"]);
		data.cutCg.interpType = obj["interp_type"].toInt();
		break;
	case PARTS_CP_DRAW_TEXT:
	case PARTS_CP_COPY_TEXT:
		data.text.text = obj["text"].toString();
		data.text.pos = Point(obj["pos"]);
		data.text.lineSpace = obj["line_space"].toInt();
		data.text.style = TextStyle(obj["style"]);
		break;
	case PARTS_CP_INVALID:
		break;
	}
}

PartsCpOp::PartsCpOp(const PartsCpOp &other)
	: data(PARTS_CP_INVALID)
{
	type = other.type;
	switch (type) {
	case PARTS_CP_CREATE:
	case PARTS_CP_CREATE_PIXEL_ONLY:
		new(&data.create) PartsCpCreate(other.data.create);
		break;
	case PARTS_CP_CG:
		new(&data.cg) PartsCpCg(other.data.cg);
		break;
	case PARTS_CP_FILL:
	case PARTS_CP_FILL_ALPHA_COLOR:
	case PARTS_CP_FILL_AMAP:
		new(&data.fill) PartsCpFill(other.data.fill);
		break;
	case PARTS_CP_DRAW_CUT_CG:
	case PARTS_CP_COPY_CUT_CG:
		new(&data.cutCg) PartsCpCutCg(other.data.cutCg);
		break;
	case PARTS_CP_DRAW_TEXT:
	case PARTS_CP_COPY_TEXT:
		new(&data.text) PartsCpText(other.data.text);
		break;
	case PARTS_CP_INVALID:
		break;
	}
}

PartsCpOp::~PartsCpOp()
{
	switch (type) {
	case PARTS_CP_CREATE:
	case PARTS_CP_CREATE_PIXEL_ONLY:
		data.create.~PartsCpCreate();
		break;
	case PARTS_CP_CG:
		data.cg.~PartsCpCg();
		break;
	case PARTS_CP_FILL:
	case PARTS_CP_FILL_ALPHA_COLOR:
	case PARTS_CP_FILL_AMAP:
		data.fill.~PartsCpFill();
		break;
	case PARTS_CP_DRAW_CUT_CG:
	case PARTS_CP_COPY_CUT_CG:
		data.cutCg.~PartsCpCutCg();
		break;
	case PARTS_CP_DRAW_TEXT:
	case PARTS_CP_COPY_TEXT:
		data.text.~PartsCpText();
		break;
	case PARTS_CP_INVALID:
		break;
	}
}

PartsState::PartsStateData::PartsStateData(PartsType type)
{
	switch (type) {
	case PARTS_CG:
		new(&cg) PartsCg();
		break;
	case PARTS_TEXT:
		new(&text) PartsText();
		break;
	case PARTS_ANIMATION:
		new(&anim) PartsAnimation();
		break;
	case PARTS_NUMERAL:
		new(&num) PartsNumeral();
		break;
	case PARTS_HGAUGE:
	case PARTS_VGAUGE:
		new(&gauge) PartsGauge();
		break;
	case PARTS_CONSTRUCTION_PROCESS:
		new(&cproc) PartsConstructionProcess();
		break;
	case PARTS_INVALID:
	case PARTS_UNINITIALIZED:
		break;
	}
};

PartsState::PartsState(const QJsonObject &obj)
	: data(stringToPartsType(obj["type"].toString()))
{
	type = stringToPartsType(obj["type"].toString());
	switch (type) {
	case PARTS_CG:
		data.cg.no = obj["no"].toInt();
		break;
	case PARTS_TEXT:
		for (const QJsonValue &val : obj["lines"].toArray()) {
			QJsonObject line = val.toObject();
			data.text.lines.append({
				.contents = line["contents"].toString(),
				.width = line["width"].toInt(),
				.height = line["height"].toInt()
			});
		}
		data.text.lineSpace = obj["line_space"].toInt();
		data.text.cursor = Point(obj["cursor"]);
		data.text.textStyle = TextStyle(obj["text_style"]);
		break;
	case PARTS_ANIMATION:
		data.anim.startNo = obj["start_no"].toInt();
		data.anim.frameTime = obj["frame_time"].toInt();
		data.anim.elapsed = obj["elapsed"].toInt();
		data.anim.currentFrame = obj["current_frame"].toInt();
		// TODO: frames?
		break;
	case PARTS_NUMERAL:
		data.num.haveNum = obj["have_num"].toBool();
		data.num.num = obj["num"].toInt();
		data.num.space = obj["space"].toInt();
		data.num.showComma = obj["show_comma"].toBool();
		data.num.length = obj["length"].toInt();
		data.num.cgNo = obj["cg_no"].toInt();
		// TODO: cg?
		break;
	case PARTS_HGAUGE:
	case PARTS_VGAUGE:
		// TODO: cg?
		break;
	case PARTS_CONSTRUCTION_PROCESS:
		for (const QJsonValue &val : obj["operations"].toArray()) {
			data.cproc.operations.append(PartsCpOp(val.toObject()));
		}
		break;
	case PARTS_INVALID:
	case PARTS_UNINITIALIZED:
		break;
	}
}

PartsState::PartsState(const PartsState &other)
	: data(PARTS_INVALID)
{
	type = other.type;
	switch (type) {
	case PARTS_CG:
		new(&data.cg) PartsCg(other.data.cg);
		break;
	case PARTS_TEXT:
		new(&data.text) PartsText(other.data.text);
		break;
	case PARTS_ANIMATION:
		new(&data.anim) PartsAnimation(other.data.anim);
		break;
	case PARTS_NUMERAL:
		new(&data.num) PartsNumeral(other.data.num);
		break;
	case PARTS_HGAUGE:
	case PARTS_VGAUGE:
		new(&data.gauge) PartsGauge(other.data.gauge);
		break;
	case PARTS_CONSTRUCTION_PROCESS:
		new(&data.cproc) PartsConstructionProcess(other.data.cproc);
		break;
	case PARTS_INVALID:
	case PARTS_UNINITIALIZED:
		break;
	}
}

PartsState::~PartsState()
{
	switch (type) {
	case PARTS_CG:
		data.cg.~PartsCg();
		break;
	case PARTS_TEXT:
		data.text.~PartsText();
		break;
	case PARTS_ANIMATION:
		data.anim.~PartsAnimation();
		break;
	case PARTS_NUMERAL:
		data.num.~PartsNumeral();
		break;
	case PARTS_HGAUGE:
	case PARTS_VGAUGE:
		data.gauge.~PartsGauge();
		break;
	case PARTS_CONSTRUCTION_PROCESS:
		data.cproc.~PartsConstructionProcess();
		break;
	case PARTS_INVALID:
	case PARTS_UNINITIALIZED:
		break;

	}
}

QString PartsState::description()
{
	switch (type) {
	case PARTS_CG:
		return QString("CG %1").arg(data.cg.no);
	case PARTS_TEXT:
		return "Text";
	case PARTS_ANIMATION:
		return "Animation";
	case PARTS_NUMERAL:
		if (data.num.haveNum)
			return QString("Numeral %1").arg(data.num.num);
		return "Numeral (uninitialized)";
	case PARTS_HGAUGE:
		return "HGauge";
	case PARTS_VGAUGE:
		return "VGauge";
	case PARTS_CONSTRUCTION_PROCESS:
		return "Construction Process";
	case PARTS_UNINITIALIZED:
		return "Uninitialized";
	case PARTS_INVALID:
		return "<invalid>";
	}
	return "<invalid>";
}

PartsParams::PartsParams(const QJsonObject &obj)
	: pos(obj["pos"])
	, scale(obj["scale"])
	, rotation(obj["rotation"])
	, addColor(obj["add_color"])
	, mulColor(obj["mul_color"])
{
	z = obj["z"].toInt();
	show = obj["show"].toBool();
	alpha = obj["alpha"].toInt();
}

static PartsMotionType stringToMotionType(const QString &name)
{
	if (name == "pos")
		return PARTS_MOTION_POS;
	if (name == "alpha")
		return PARTS_MOTION_ALPHA;
	if (name == "cg")
		return PARTS_MOTION_CG;
	if (name == "hgauge_rate")
		return PARTS_MOTION_HGAUGE_RATE;
	if (name == "vgauge_rate")
		return PARTS_MOTION_VGAUGE_RATE;
	if (name == "numeral_number")
		return PARTS_MOTION_NUMERAL_NUMBER;
	if (name == "mag_x")
		return PARTS_MOTION_MAG_X;
	if (name == "mag_y")
		return PARTS_MOTION_MAG_Y;
	if (name == "rotate_x")
		return PARTS_MOTION_ROTATE_X;
	if (name == "rotate_y")
		return PARTS_MOTION_ROTATE_Y;
	if (name == "rotate_z")
		return PARTS_MOTION_ROTATE_Z;
	if (name == "vibration_size")
		return PARTS_MOTION_VIBRATION_SIZE;
	return PARTS_MOTION_INVALID;
}

PartsMotion::PartsMotion(const QJsonObject &obj)
{
	type = stringToMotionType(obj["type"].toString());

	switch (type) {
	case PARTS_MOTION_POS:
		begin.pos.x = obj["begin"].toObject()["x"].toInt();
		begin.pos.y = obj["begin"].toObject()["y"].toInt();
		end.pos.x = obj["end"].toObject()["x"].toInt();
		end.pos.y = obj["end"].toObject()["y"].toInt();
		break;
	case PARTS_MOTION_VIBRATION_SIZE:
		begin.dim.w = obj["begin"].toObject()["w"].toInt();
		begin.dim.h = obj["begin"].toObject()["h"].toInt();
		end.dim.w = obj["end"].toObject()["w"].toInt();
		end.dim.h = obj["end"].toObject()["h"].toInt();
		break;
	case PARTS_MOTION_ALPHA:
	case PARTS_MOTION_CG:
	case PARTS_MOTION_NUMERAL_NUMBER:
		begin.i = obj["begin"].toInt();
		end.i = obj["end"].toInt();
		break;
	case PARTS_MOTION_HGAUGE_RATE:
	case PARTS_MOTION_VGAUGE_RATE:
	case PARTS_MOTION_MAG_X:
	case PARTS_MOTION_MAG_Y:
	case PARTS_MOTION_ROTATE_X:
	case PARTS_MOTION_ROTATE_Y:
	case PARTS_MOTION_ROTATE_Z:
		begin.f = obj["begin"].toDouble();
		end.f = obj["end"].toDouble();
		break;
	case PARTS_MOTION_INVALID:
		break;
	}

	beginTime = obj["beginTime"].toInt();
	endTime = obj["endTime"].toInt();
}

Parts::Parts(const QJsonObject &obj, Parts *parentPtr)
	: deflt(obj["default"].toObject())
	, hovered(obj["hovered"].toObject())
	, clicked(obj["clicked"].toObject())
	, local(obj["local"].toObject())
	, global(obj["global"].toObject())
{
	parent = parentPtr;
	no = obj["no"].toInt();
	state = obj["state"].toString();
	delegateIndex = obj["delegate_index"].toInt();
	spriteDeform = obj["sprite_deform"].toInt();
	clickable = obj["clickable"].toBool();
	onCursorSound = obj["on_cursor_sound"].toInt();
	onClickSound = obj["on_click_sound"].toInt();
	originMode = obj["origin_mode"].toInt();
	linkedTo = obj["linked_to"].toInt();
	linkedFrom = obj["linked_from"].toInt();
	drawFilter = obj["draw_filter"].toInt();
	for (const QJsonValue &val : obj["motions"].toArray()) {
		motions.append(PartsMotion(val.toObject()));
	}
	for (const QJsonValue &val : obj["children"].toArray()) {
		children.append(Parts(val.toObject(), this));
	}
}

// XXX: we need a copy constructor to update parent pointers when QVectors are copied
Parts::Parts(const Parts &other)
	: no(other.no)
	, state(other.state)
	, deflt(other.deflt)
	, hovered(other.hovered)
	, clicked(other.clicked)
	, local(other.local)
	, global(other.global)
	, delegateIndex(other.delegateIndex)
	, spriteDeform(other.spriteDeform)
	, clickable(other.clickable)
	, onCursorSound(other.onCursorSound)
	, onClickSound(other.onClickSound)
	, originMode(other.originMode)
	, linkedTo(other.linkedTo)
	, linkedFrom(other.linkedFrom)
	, drawFilter(other.drawFilter)
	, motions(other.motions)
	, children(other.children)
	, parent(other.parent)
{
	for (Parts &child : children) {
		child.parent = this;
	}
}

QString Parts::description()
{
	if (state == "hovered")
		return hovered.description();
	if (state == "clicked")
		return clicked.description();
	return deflt.description();
}

Sprite::Sprite(const QJsonValue &val)
{
	if (!val.isObject()) {
		qDebug() << "invalid Sprite object:" << val;
		no = -1;
		return;
	}

	QJsonObject obj = val.toObject();
	no = obj["no"].toInt();
	color = Color(obj["color"]);
	multiply_color = Color(obj["multiply_color"]);
	add_color = Color(obj["add_color"]);
	blend_rate = obj["blend_rate"].toInt();
	draw_method = obj["draw_method"].toString();
	rect = Rectangle(obj["rect"]);
	cg_no = obj["cg_no"].toInt();
}

SceneEntity::SceneEntity(const QJsonValue &val)
{
	if (!val.isObject()) {
		qDebug() << "invalid SceneEntity object:" << val;
		name = "<invalid>";
		id = -1;
		sprite.no = -1;
		return;
	}

	QJsonObject obj = val.toObject();
	name = "<anonymous entity>";
	id = obj["id"].toInt();
	z = obj["z"].toInt();
	z2 = obj["z2"].toInt();

	// SACT2/Stoat/Chipmunk sprite
	if (obj.contains("sprite")) {
		sprite = Sprite(obj["sprite"]);
		name = QString("sprite %1").arg(sprite.no);
	} else {
		sprite.no = -1;
	}

	// GoatGUIEngine/GUIEngine/PartsEngine
	if (obj.contains("parts")) {
		for (const QJsonValue &val : obj["parts"].toArray()) {
			parts.append(Parts(val.toObject()));
		}
		name = "PartsEngine";
	}
}

QPixmap parseTexture(const QJsonValue &val)
{
	if (!val.isObject()) {
		qDebug() << "invalid texture object";
		return QPixmap();
	}
	QJsonObject obj = val.toObject();
	int width = obj["width"].toInt();
	int height = obj["height"].toInt();
	QByteArray b64 = obj["pixels"].toString().toLatin1();
	QByteArray pixels = QByteArray::fromBase64(b64, QByteArray::Base64Encoding);
	if (pixels.size() < width * height * 4) {
		qDebug() << "pixel data truncated?";
		return QPixmap();
	}

	QImage image((uchar*)pixels.data(), width, height, width * 4,
			QImage::Format_RGBA8888);
	return QPixmap::fromImage(image);
}
