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


#include <QDebug>
#include <QtWidgets>

#include "codeviewer.hpp"
#include "syntaxhighlighter.hpp"
#include "variablesmodel.hpp"

extern "C" {
#include <string.h>
#include "system4/ain.h"
#include "system4/dasm.h"
#include "system4/instructions.h"
#include "system4/string.h"
}

CodeArea::CodeArea(QWidget *parent) : QPlainTextEdit(parent)
{
	breakpointImage.load(":/icons/debug-breakpoint-stackframe-dot.svg");

	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	font.setFixedPitch(true);
	font.setPointSize(10);
	setFont(font);

	highlighter = new SyntaxHighlighter(document());

	QTextCharFormat fmt;

	fmt.setForeground(Qt::blue);
	fmt.setFontWeight(QFont::Bold);
	highlighter->addRule(QRegularExpression(QStringLiteral("\\bFUNC\\b")), fmt);
	highlighter->addRule(QRegularExpression(QStringLiteral("\\bENDFUNC\\b")), fmt);
	fmt.setFontWeight(QFont::Normal);

	fmt.setForeground(Qt::darkCyan);
	highlighter->addRule(QRegularExpression(QStringLiteral("\\b0x[a-fA-F0-9]+\\b")), fmt);
	highlighter->addRule(QRegularExpression(QStringLiteral("\\b[1-9][0-9]*\\b")), fmt);
	highlighter->addRule(QRegularExpression(QStringLiteral("\\b0[0-7]*\\b")), fmt);
	highlighter->addRule(QRegularExpression(QStringLiteral("\\b[0-9]+\\.[0-9]+\\b")), fmt);

	fmt.setForeground(Qt::darkGray);
	highlighter->addRule(QRegularExpression(QStringLiteral("^\\S+:")), fmt);
	highlighter->addRule(QRegularExpression(QStringLiteral("^\\.CASE\\b")), fmt);

	fmt.setForeground(Qt::red);
	highlighter->addRule(QRegularExpression(QStringLiteral("\"(\\\\.|[^\"\\\\])*\"")), fmt);

	fmt.setForeground(Qt::darkGreen);
	highlighter->addRule(QRegularExpression(QStringLiteral(";[^\n]*")), fmt);

	addressArea = new AddressArea(this);

	connect(this, &CodeArea::updateRequest, this, &CodeArea::updateAddressArea);
	connect(&Debugger::getInstance(), &Debugger::breakpointsReceived,
			this, &CodeArea::updateBreakpoints);

	setReadOnly(true);
}

#define H_PAD 2

int CodeArea::addressAreaWidth()
{
	int charWidth = fontMetrics().horizontalAdvance(QLatin1Char('9'));
	int iconHeight = fontMetrics().height();
	return iconHeight + H_PAD + (charWidth * 8);
}

void CodeArea::updateAddressAreaWidth(int newBlockCount)
{
	setViewportMargins(addressAreaWidth(), 0, 0, 0);
}

void CodeArea::updateAddressArea(const QRect &rect, int dy)
{
	if (dy)
		addressArea->scroll(0, dy);
	else
		addressArea->update(0, rect.y(), addressArea->width(), rect.height());

	if (rect.contains(viewport()->rect()))
		updateAddressAreaWidth(0);

}

void CodeArea::resizeEvent(QResizeEvent *e)
{
	QPlainTextEdit::resizeEvent(e);

	QRect cr = contentsRect();
	addressArea->setGeometry(QRect(cr.left(), cr.top(), addressAreaWidth(), cr.height()));
}

void CodeArea::addressAreaPaintEvent(QPaintEvent *event)
{
	QPainter painter(addressArea);
	painter.fillRect(event->rect(), Qt::lightGray);

	QTextBlock block = firstVisibleBlock();
	int blockNumber = block.blockNumber();
	int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
	int bottom = top + qRound(blockBoundingRect(block).height());

	while (block.isValid() && top <= event->rect().bottom()) {
		if (block.isVisible() && bottom >= event->rect().top()) {
			QString number;
			if (blockNumber < instructions.size()) {
				number = QString("%1").arg(
						(long)instructions[blockNumber].address,
						8, 16, (QChar)'0');
			} else {
				number = "";
			}
			painter.setPen(Qt::darkGray);
			painter.drawText(0, top, addressArea->width() - H_PAD,
					fontMetrics().height(), Qt::AlignRight, number);
			if (instructions[blockNumber].isBreakpoint) {
				int size = bottom - top;
				painter.drawPixmap(0, top, size, size, breakpointImage);
			}
		}

		block = block.next();
		top = bottom;
		bottom = top + qRound(blockBoundingRect(block).height());
		++blockNumber;
	}
}

void CodeArea::addressAreaContextMenuEvent(QContextMenuEvent *event)
{
	QTextBlock block = firstVisibleBlock();
	int blockNumber = block.blockNumber();
	int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
	int bottom = top + qRound(blockBoundingRect(block).height());
	int y = event->y();

	uint32_t addr = 0xFFFFFFFF;
	while (block.isValid()) {
		if (y >= top && y <= bottom) {
			addr = instructions[blockNumber].address;
			break;
		}
		block = block.next();
		top = bottom;
		bottom = top + qRound(blockBoundingRect(block).height());
		++blockNumber;
	}

	if (addr == 0xFFFFFFFF)
		return;

	QMenu menu(addressArea);

	QAction toggleBPAct(tr("&Toggle Breakpoint"));
	toggleBPAct.connect(&toggleBPAct, &QAction::triggered, this,
			[this, addr]{ toggleBreakpoint(addr); });

	menu.addAction(&toggleBPAct);
	menu.exec(event->globalPos());
}

void CodeArea::updateBreakpoints(QSet<uint32_t> &breakpoints)
{
	for (Instruction &instr : instructions) {
		instr.isBreakpoint = breakpoints.contains(instr.address);
	}
	update();
}

void CodeArea::toggleBreakpoint(uint32_t addr)
{
	Debugger::getInstance().toggleInstructionBreakpoint(addr);
}

static char escape_char(char c)
{
	switch (c) {
	case '\\': return '\\';
	case '\"': return '\"';
	case '\n': return 'n';
	case '\r': return 'r';
	default:   return 0;
	}
}

static QString escape_string(const char *str)
{
	unsigned escapes = 0;

	// count number of required escapes
	for (int i = 0; str[i]; i++) {
		if (str[i] & 0x80)
			continue;
		if (escape_char(str[i])) {
			escapes++;
			break;
		}
	}

	if (!escapes)
		return strdup(str);

	// add backslash escapes
	int dst = 0;
	char *out = (char*)xmalloc(strlen(str) + escapes + 1);
	for (int i = 0; str[i]; i++) {
		char c = escape_char(str[i]);
		if (c) {
			out[dst++] = '\\';
			out[dst++] = c;
		} else {
			out[dst++] = str[i];
		}
	}
	out[dst] = '\0';

	QString r = QString::fromUtf8(out);
	free(out);
	return r;
}

static QString string_literal(const char *str)
{
	QString out = escape_string(str);
	out.prepend("\"");
	out.append("\"");
	return out;
}

static QString identifier(const char *str)
{
	if (strchr(str, ' '))
		return escape_string(str);
	return str;
}

static QString local_variable(struct ain *ain, struct ain_function *f, int32_t varno)
{
	int dup_no = 0; // nr of duplicate-named variables preceding varno
	for (int i = 0; i < varno; i++) {
		if (!strcmp(f->vars[i].name, f->vars[varno].name))
			dup_no++;
	}

	// if variable name is ambiguous, add #n suffix
	if (dup_no)
		return QString("%1#%2").arg(f->vars[varno].name, dup_no);
	return f->vars[varno].name;
}

static QString function_name(struct ain *ain, struct ain_function *func)
{
	int i = ain_get_function_index(ain, func);
	if (!i)
		return func->name;

	QString out = QString::fromUtf8(func->name);
	out.append(QString("#%1").arg(i));
	return out;
}

static QString arg_to_string(struct ain *ain, int fno, int32_t arg, int argtype)
{
	switch (argtype) {
	case T_INT:
	case T_SWITCH:
		return QString::number((int)arg);
	case T_FLOAT: {
		union { int32_t i; float f; } cast = { .i = arg };
		return QString::number(cast.f);
	}
	case T_ADDR:
		return QString("0x%1").arg((long)arg, 8, 16, (QChar)'0');
	case T_FUNC:
		if (arg < 0 || arg >= ain->nr_functions)
			return QString("<invalid function: %1>").arg(arg);
		return function_name(ain, &ain->functions[arg]);
	case T_DLG:
		if (arg < 0 || arg >= ain->nr_delegates)
			return QString("<invalid delegate: %1>").arg(arg);
		return identifier(ain->delegates[arg].name);
	case T_STRING:
		if (arg < 0 || arg >= ain->nr_strings)
			return QString("<invalid string: %1>").arg(arg);
		return string_literal(ain->strings[arg]->text);
	case T_MSG:
		if (arg < 0 || arg >= ain->nr_messages)
			return QString("<invalid message: %1>").arg(arg);
		return string_literal(ain->messages[arg]->text);
	case T_LOCAL:
		if (arg < 0 || arg >= ain->functions[fno].nr_vars)
			return QString("<invalid local: %1>").arg(arg);
		return local_variable(ain, &ain->functions[fno], arg);
	case T_GLOBAL:
		if (arg < 0 || arg >= ain->nr_globals)
			return QString("<invalid global: %1>").arg(arg);
		return identifier(ain->globals[arg].name);
	case T_STRUCT:
		if (arg < 0 || arg >= ain->nr_structures)
			return QString("<invalid struct: %1>").arg(arg);
		return identifier(ain->structures[arg].name);
	case T_SYSCALL:
		if (arg < 0 || arg >= NR_SYSCALLS || !syscalls[arg].name)
			return QString("<invalid/unknown syscall: %1>").arg(arg);
		return syscalls[arg].name;
	case T_HLL:
		if (arg < 0 || arg >= ain->nr_libraries)
			return QString("<invalid library: %1>").arg(arg);
		return identifier(ain->libraries[arg].name);
	case T_HLLFUNC:
		return QString::number(arg);
	case T_FILE:
		if (!ain->nr_filenames)
			return QString::number(arg);
		if (arg < 0 || arg >= ain->nr_filenames)
			return QString("<invalid file: %1>").arg(arg);
		return string_literal(ain->filenames[arg]);
	default:
		return QString("<unknown arg type (%1): %2>").arg(argtype, arg);
	}
}

static QString hll_function_name(struct ain *ain, int lib_no, int func_no)
{
	if (lib_no < 0 || lib_no >= ain->nr_libraries)
		return QString::number(func_no);
	if (func_no < 0 || func_no >= ain->libraries[lib_no].nr_functions)
		return QString("<invalid library function: %1>").arg(func_no);

	struct ain_library *lib = &ain->libraries[lib_no];

	int dup_no = 0;
	for (int i = 0;i < lib->nr_functions; i++) {
		if (i == func_no)
			break;
		if (!strcmp(lib->functions[i].name, lib->functions[func_no].name))
			dup_no++;
	}

	if (!dup_no)
		return lib->functions[func_no].name;

	QString out = QString::fromUtf8(lib->functions[func_no].name);
	out.append(QString("#%1").arg(dup_no));
	return out;
}

QString CodeArea::Instruction::toString(struct ain *ain, int fno)
{
	QString str = instr->name;
	for (int i = 0; i < instr->nr_args; i++) {
		// XXX: special case: T_HLLFUNC is context-dependent
		if (instr->args[i] == T_HLLFUNC && i > 0 && instr->args[i-1] == T_HLL) {
			str += QString(".%1").arg(hll_function_name(ain, args[i-1], args[i]));
			continue;
		}
		str += QString(" %1").arg(arg_to_string(ain, fno, args[i], instr->args[i]));
	}
	return str + "\n";
}

static bool dasm_finished(struct dasm *dasm)
{
	return dasm_eof(dasm) || dasm_opcode(dasm) == ENDFUNC || dasm_opcode(dasm) == FUNC;
}

void CodeArea::pushInstruction(struct dasm *dasm)
{
	uint32_t addr = dasm_addr(dasm);
	Instruction instr = {
		.address = addr,
		.isBreakpoint = Debugger::getInstance().isBreakpoint(addr),
		.instr = dasm_instruction(dasm)
	};
	for (int i = 0; i < instr.instr->nr_args; i++) {
		instr.args[i] = dasm_arg(dasm, i);
	}
	instructions.push_back(instr);
}

bool CodeArea::setFunction(struct ain *ain, int fno, int address)
{
	if (fno < 0 || fno >= ain->nr_functions)
		return false;

	struct ain_function *f = &ain->functions[fno];

	struct dasm dasm;
	dasm_init(&dasm, ain);
	dasm_jump(&dasm, f->address - 6);

	instructions.clear();
	do {
		pushInstruction(&dasm);
		dasm_next(&dasm);
	} while (!dasm_finished(&dasm));

	if (dasm_opcode(&dasm) == ENDFUNC) {
		pushInstruction(&dasm);
	}

	int line = -1;
	QString contents = "";
	for (int i = 0; i < instructions.size(); i++) {
		contents += instructions[i].toString(ain, fno);
		if (instructions[i].address == (uint32_t)address) {
			line = i;
		}
	}

	setPlainText(contents);

	if (line >= 0) {
		QTextEdit::ExtraSelection selection;
		QColor lineColor = QColor(Qt::yellow).lighter(160);
		selection.format.setBackground(lineColor);
		selection.format.setProperty(QTextFormat::FullWidthSelection, true);
		QTextCursor cursor(document()->findBlockByLineNumber(line));
		selection.cursor = cursor;
		selection.cursor.clearSelection();

		QList<QTextEdit::ExtraSelection> extraSelections { selection };
		setExtraSelections(extraSelections);

		// scroll to instruction
		setTextCursor(cursor);
	} else {
		QList<QTextEdit::ExtraSelection> extraSelections;
		setExtraSelections(extraSelections);
	}

	emit functionChanged(fno);
	return true;
}

bool CodeArea::setFunction(struct ain *ain, const char *name, int address)
{
	char *tmp = strdup(name);
	int fno = ain_get_function(ain, tmp);
	free(tmp);
	return setFunction(ain, fno, address);
}

CodeViewer::CodeViewer(QWidget *parent)
	: QSplitter(parent)
{
	codeArea = new CodeArea;
	frameSelector = new QComboBox;
	stack = new QStackedWidget;

	addWidget(codeArea);

	QWidget *rightPane = new QWidget;
	QVBoxLayout *layout = new QVBoxLayout(rightPane);
	layout->addWidget(frameSelector);
	layout->addWidget(stack);
	layout->setContentsMargins(0, 0, 0, 0);
	addWidget(rightPane);

	connect(codeArea, &CodeArea::functionChanged, this, &CodeViewer::functionChanged);
	connect(&Debugger::getInstance(), &Debugger::stackTraceReceived,
			this, &CodeViewer::stackTraceReceived);
	connect(frameSelector, QOverload<int>::of(&QComboBox::activated),
			this, &CodeViewer::stackFrameChanged);
}

void CodeViewer::setAin(struct ain *a)
{
	code = a;

	// create a dummy stack trace for initial state
	QVector<Debugger::StackFrame> dummy(1);
	dummy[0].name = "main";
	dummy[0].id = 0;
	dummy[0].address = a->functions[a->main].address;
	stackTraceReceived(dummy);
}

void CodeViewer::setFunction(const QString &name)
{
	if (!code)
		return;
	codeArea->setFunction(code, name.toUtf8().constData(), 0);
}

void CodeViewer::stackTraceReceived(QVector<Debugger::StackFrame> &frames)
{
	stackTrace = frames;

	// delete widgets/models for old stack trace
	frameSelector->clear();
	while (stack->count() > 0) {
		stack->removeWidget(stack->widget(0));
	}
	for (int i = 0; i < varModels.size(); i++) {
		delete varModels[i];
	}
	varModels.clear();

	// create widgets for new stack trace
	for (Debugger::StackFrame &frame : stackTrace) {
		QString label = QString::number(frame.id) + ": " + frame.name
			+ " @ " + QString::number(frame.address, 16);
		frameSelector->addItem(label);

		QTreeView *treeView = new QTreeView;
		VariablesModel *model = new VariablesModel(frame.scopes);
		varModels.push_back(model);
		treeView->setModel(model);
		stack->addWidget(treeView);
	}

	// activate current function
	frameSelector->setCurrentIndex(frameSelector->count() - 1);
	stackFrameChanged(stackTrace.size() - 1);
}

void CodeViewer::stackFrameChanged(int i)
{
	codeArea->setFunction(code, stackTrace[i].name.toUtf8().constData(), stackTrace[i].address);
	stack->setCurrentIndex(i);
}
