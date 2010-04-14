/*
 *  Copyright 2007-2010 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef _INPUT_H
#define _INPUT_H

#include <assert.h>
#include <stdio.h>
#include <iostream>

struct exit_object { };
extern exit_object endp;
void operator<<( std::ostream &out, exit_object & );

#define FSM_BUFSIZE 8192
//#define FSM_BUFSIZE 8

/*
 * pdaRun <- fsmRun <- stream 
 *
 * Activities we need to support:
 *
 * 1. Stuff data into an input stream each time we <<
 * 2. Detach an input stream, and attach another when we include
 * 3. Send data back to an input stream when the parser backtracks
 * 4. Temporarily stop parsing due to a lack of input.
 *
 * At any given time, the fsmRun struct may have a prefix of the stream's
 * input. If getting data we first get what we can out of the fsmRun, then
 * consult the stream. If sending data back, we first shift pointers in the
 * fsmRun, then ship to the stream. If changing streams the old stream needs to
 * take back unprocessed data from the fsmRun.
 */

struct KlangEl;
struct Pattern;
struct PatternItem;
struct Replacement;
struct ReplItem;
struct RunBuf;
struct FsmRun;
typedef struct _Tree Tree;

struct RunBuf
{
	enum Type {
		DataType = 0,
		TokenType,
		IgnoreType
	};

	RunBuf()
	:
		type(DataType),
		length(0),
		tree(0),
		offset(0),
		next(0),
		prev(0)
	{}

	Type type;
	char data[FSM_BUFSIZE];
	long length;
	Tree *tree;
	long offset;
	RunBuf *next, *prev;
};

struct InputStream;

struct InputFuncs
{
	bool (*isTree)( InputStream *is );
	bool (*isIgnore)( InputStream *is );
	bool (*isLangEl)( InputStream *is );
	bool (*isEof)( InputStream *is );
	int (*needFlush)( InputStream *is );
	bool (*tryAgainLater)( InputStream *is );
	int (*getData)( InputStream *is, char *dest, int length );
	int (*getDataImpl)( InputStream *is, char *dest, int length );
	Tree *(*getTree)( InputStream *is );
	KlangEl *(*getLangEl)( InputStream *is, long &bindId, char *&data, long &length );
	void (*pushTree)( InputStream *is, Tree *tree, bool ignore );
	void (*pushText)( InputStream *is, const char *data, long len );
	Tree *(*undoPush)( InputStream *is, int length );
	void (*appendData)( InputStream *is, const char *data, long len );
	void (*appendTree)( InputStream *is, Tree *tree );
	Tree *(*undoAppend)( InputStream *is, int length );
	void (*pushBackNamed)( InputStream *is );
	void (*pushBackBuf)( InputStream *is, RunBuf *runBuf );
};

extern InputFuncs baseFuncs;
extern InputFuncs stringFuncs;
extern InputFuncs fileFuncs;
extern InputFuncs fdFuncs;
extern InputFuncs accumFuncs;
extern InputFuncs staticFuncs;
extern InputFuncs patternFuncs;
extern InputFuncs replFuncs;



struct InputStream
{
	InputStream( bool handlesLine )
	:
		hasData(0),
		eofSent(false),
		flush(false),
		eof(false),
		line(1),
		column(1),
		byte(0),
		handlesLine(handlesLine),
		later(false),
		queue(0), 
		queueTail(0)
	{
		funcs = &baseFuncs;
	}

	InputStream( const char *data, long dlen ) :
		hasData(0),
		eofSent(false),
		flush(false),
		eof(false),
		line(1),
		column(1),
		byte(0),
		handlesLine(false), 
		later(false),
		queue(0), 
		queueTail(0),
		data(data), 
		dlen(dlen),
		offset(0)
	{
		funcs = &stringFuncs;
	}

	InputStream( FILE *file ) :
		hasData(0),
		eofSent(false),
		flush(false),
		eof(false),
		line(1),
		column(1),
		byte(0),
		handlesLine(false),
		later(false),
		queue(0), 
		queueTail(0),
		file(file)
	{
		funcs = &fileFuncs;
	}

	InputStream( long fd ) :
		hasData(0),
		eofSent(false),
		flush(false),
		eof(false),
		line(1),
		column(1),
		byte(0),
		handlesLine(false),
		later(false),
		queue(0), 
		queueTail(0),
		fd(fd)
	{
		funcs = &fdFuncs;
	}

	InputStream() :
		hasData(0),
		eofSent(false),
		flush(false),
		eof(false),
		line(1),
		column(1),
		byte(0),
		handlesLine(false),
		later(false),
		queue(0), 
		queueTail(0),
		offset(0)
	{
		funcs = &accumFuncs;
	}

	InputStream( Pattern *pattern );
	InputStream( Replacement *replacement );

	InputFuncs *funcs;

	/* Basic functions. */
	
	FsmRun *hasData;

	bool eofSent;
	bool flush;
	bool eof;

	long line;
	long column;
	long byte;

	/* This is set true for input streams that do their own line counting.
	 * Causes FsmRun to ignore NLs. */
	bool handlesLine;
	bool later;

	RunBuf *queue;
	RunBuf *queueTail;

	RunBuf *head() { return queue; }
	RunBuf *tail() { return queueTail; }

	const char *data;
	long dlen;
	int offset;

	FILE *file;
	long fd;

	RunBuf *popHead();
	RunBuf *popTail();
	void append( RunBuf *runBuf );
	void prepend( RunBuf *runBuf );

	Pattern *pattern;
	PatternItem *patItem;
	Replacement *replacement;
	ReplItem *replItem;
};

#endif /* _INPUT_H */

