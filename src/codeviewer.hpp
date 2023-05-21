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

#ifndef XSYS4DBG_CODEVIEWER_HPP
#define XSYS4DBG_CODEVIEWER_HPP

#include <QPlainTextEdit>
#include <QSet>
#include <QSplitter>
#include <QVector>
#include "debugger.hpp"

class QComboBox;
class QContextMenuEvent;
class QPaintEvent;
class QResizeEvent;
class QSize;
class QStackedWidget;
class QWidget;

struct ain;
struct dasm;
class SyntaxHighlighter;
class VariablesModel;

extern "C" {
#include "system4/instructions.h"
}

class CodeArea : public QPlainTextEdit
{
	Q_OBJECT
public:
	CodeArea(QWidget *parent = nullptr);

	void addressAreaPaintEvent(QPaintEvent *event);
	void addressAreaContextMenuEvent(QContextMenuEvent *event);
	int addressAreaWidth();

	bool setFunction(struct ain *ain, int fno, int address);
	bool setFunction(struct ain *ain, const char *name, int address);

signals:
	void functionChanged(int fno);

protected:

	void resizeEvent(QResizeEvent *event) override;

private slots:
	void updateAddressAreaWidth(int newBlockCount);
	void updateAddressArea(const QRect &rect, int dy);
	void updateBreakpoints(QSet<uint32_t> &breakpoints);
	void toggleBreakpoint(uint32_t addr);

private:

	struct Instruction {
		uint32_t address;
		bool isBreakpoint;
		const struct instruction *instr;
		int32_t args[INSTRUCTION_MAX_ARGS];
		QString toString(struct ain *ain, int fno);
	};

	void pushInstruction(struct dasm *dasm);

	QVector<Instruction> instructions;

	QWidget *addressArea;
	SyntaxHighlighter *highlighter;

	QPixmap breakpointImage;
};

class AddressArea : public QWidget
{
	Q_OBJECT
public:
	AddressArea(CodeArea *code) : QWidget(code), codeArea(code)
	{}

	QSize sizeHint() const override
	{
		return QSize(codeArea->addressAreaWidth(), 0);
	}

protected:
	void paintEvent(QPaintEvent *event) override
	{
		codeArea->addressAreaPaintEvent(event);
	}
	void contextMenuEvent(QContextMenuEvent *event) override
	{
		codeArea->addressAreaContextMenuEvent(event);
	}

private:
	CodeArea *codeArea;
};

class CodeViewer : public QSplitter
{
	Q_OBJECT
public:
	CodeViewer(QWidget *parent = nullptr);
	~CodeViewer();

	void setAin(struct ain *a);
	void setFunction(const QString &name);

signals:
	void functionChanged(int fno);

private slots:
	void stackTraceReceived(QVector<Debugger::StackFrame> &frames);
	void stackFrameChanged(int index);

private:
	struct ain *code = NULL;
	QVector<Debugger::StackFrame> stackTrace;
	QVector<VariablesModel*> varModels;
	CodeArea *codeArea;
	QComboBox *frameSelector;
	QStackedWidget *stack;
};

#endif
