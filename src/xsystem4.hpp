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

#ifndef XSYS4DBG_XSYSTEM4_HPP
#define XSYS4DBG_XSYSTEM4_HPP

#include <optional>
#include <QVector>
#include <QPixmap>

class QJsonObject;
class QJsonValue;

struct Color {
	Color() {};
	Color(const QJsonValue &val);
	QString toString() const;
	int r, g, b, a;
};

struct Rectangle {
	Rectangle() {};
	Rectangle(const QJsonValue &val);
	QString toString() const;
	int x, y, w, h;
};

struct Point {
	Point() {};
	Point(const QJsonValue &val);
	QString toString() const;
	int x, y;
};

struct Point3D {
	Point3D() {};
	Point3D(const QJsonValue &val);
	QString toString() const;
	int x, y, z;
};

struct Size {
	Size() {};
	Size(const QJsonValue &val);
	QString toString() const;
	int w, h;
};

struct TextStyle {
	TextStyle() {};
	TextStyle(const QJsonValue &val);
	unsigned face;
	float size;
	float bold_width;
	unsigned weight;
	float edge_left;
	float edge_up;
	float edge_right;
	float edge_down;
	Color color;
	Color edge_color;
	float scale_x;
	float space_scale_x;
	float font_spacing;
};

enum PartsType {
	PARTS_INVALID,
	PARTS_UNINITIALIZED,
	PARTS_CG,
	PARTS_TEXT,
	PARTS_ANIMATION,
	PARTS_NUMERAL,
	PARTS_HGAUGE,
	PARTS_VGAUGE,
	PARTS_CONSTRUCTION_PROCESS,
	PARTS_FLASH,
};

struct PartsCg {
	int no;
};

struct PartsTextLine {
	QString contents;
	int width;
	int height;
};

struct PartsText {
	QVector<PartsTextLine> lines;
	int lineSpace;
	Point cursor;
	TextStyle textStyle;
};

struct PartsAnimation {
	int startNo;
	int frameTime;
	int elapsed;
	int currentFrame;
	// TODO: frames?
};

struct PartsNumeral {
	bool haveNum;
	int num;
	int space;
	bool showComma;
	int length;
	int cgNo;
	// TODO: cg?
};

struct PartsGauge {
	// TODO: cg?
};

enum PartsCpType {
	PARTS_CP_INVALID,
	PARTS_CP_CREATE,
	PARTS_CP_CREATE_PIXEL_ONLY,
	PARTS_CP_CG,
	PARTS_CP_FILL,
	PARTS_CP_FILL_ALPHA_COLOR,
	PARTS_CP_FILL_AMAP,
	PARTS_CP_DRAW_CUT_CG,
	PARTS_CP_COPY_CUT_CG,
	PARTS_CP_DRAW_TEXT,
	PARTS_CP_COPY_TEXT,
};

struct PartsCpCreate {
	int width;
	int height;
};

struct PartsCpCg {
	int no;
};

struct PartsCpFill {
	Rectangle rect;
	Color color;
};

struct PartsCpCutCg {
	int cgNo;
	Rectangle dst;
	Rectangle src;
	int interpType;
};

struct PartsCpText {
	QString text;
	Point pos;
	int lineSpace;
	TextStyle style;
};

struct PartsCpOp {
	PartsCpOp(const QJsonObject &obj);
	PartsCpOp(const PartsCpOp &other);
	~PartsCpOp();
	PartsCpType type;
	union PartsCpOpData {
		PartsCpOpData(PartsCpType type);
		~PartsCpOpData() {};
		PartsCpCreate create;
		PartsCpCg cg;
		PartsCpFill fill;
		PartsCpCutCg cutCg;
		PartsCpText text;
	} data;
};

struct PartsConstructionProcess {
	QVector<PartsCpOp> operations;
};

struct PartsFlash {
	QString filename;
	int frame_count;
	int current_frame;
};

struct PartsState {
	PartsState(const QJsonObject &obj);
	PartsState(const PartsState &other);
	~PartsState();
	QString description();
	PartsType type;
	Size size;
	Point originOffset;
	Rectangle hitbox;
	Rectangle surfaceArea;
	union PartsStateData {
		PartsStateData(PartsType type);
		~PartsStateData() {};
		PartsCg cg;
		PartsText text;
		PartsAnimation anim;
		PartsNumeral num;
		PartsGauge gauge;
		PartsConstructionProcess cproc;
		PartsFlash flash;
	} data;
};

struct PartsParams {
	PartsParams(const QJsonObject &obj);
	int z;
	Point pos;
	bool show;
	int alpha;
	Point scale;
	Point3D rotation;
	Color addColor;
	Color mulColor;
};

enum PartsMotionType {
	PARTS_MOTION_INVALID,
	PARTS_MOTION_POS,
	PARTS_MOTION_VIBRATION_SIZE,
	PARTS_MOTION_ALPHA,
	PARTS_MOTION_CG,
	PARTS_MOTION_NUMERAL_NUMBER,
	PARTS_MOTION_HGAUGE_RATE,
	PARTS_MOTION_VGAUGE_RATE,
	PARTS_MOTION_MAG_X,
	PARTS_MOTION_MAG_Y,
	PARTS_MOTION_ROTATE_X,
	PARTS_MOTION_ROTATE_Y,
	PARTS_MOTION_ROTATE_Z,
};

union PartsMotionParam {
	PartsMotionParam() {};
	Point pos;
	struct { int w, h; } dim;
	int i;
	double f;
};

struct PartsMotion {
	PartsMotion(const QJsonObject &obj);
	PartsMotionType type;
	PartsMotionParam begin;
	PartsMotionParam end;
	int beginTime;
	int endTime;
};

struct Parts {
	Parts(const QJsonObject &obj, Parts *parentPtr = nullptr);
	Parts(const Parts &other);
	bool operator==(const Parts &other) { return other.no == no; };
	QString description();
	int no;
	QString state;
	PartsState deflt;
	PartsState hovered;
	PartsState clicked;
	PartsParams local;
	PartsParams global;
	int delegateIndex;
	int spriteDeform;
	bool clickable;
	int onCursorSound;
	int onClickSound;
	int originMode;
	int linkedTo;
	int linkedFrom;
	int drawFilter;
	bool messageWindow;
	QVector<PartsMotion> motions;
	QVector<Parts> children;
	Parts *parent;
};

struct Sprite {
	Sprite() : no(-1) {};
	Sprite(const QJsonValue &val);
	int no;
	struct Color color;
	struct Color multiply_color;
	struct Color add_color;
	int blend_rate;
	QString draw_method;
	struct Rectangle rect;
	int cg_no;
	// TODO: text
};

struct SceneEntity {
	SceneEntity() : name("<empty>"), id(-1), z(0), z2(0) {};
	SceneEntity(const QJsonValue &val);
	QString name;
	int id;
	int z;
	int z2;
	std::optional<struct Sprite> sprite;
	std::optional<Parts> part;
	QVector<Parts> parts;
};

QPixmap parseTexture(const QJsonValue &val);

#endif
