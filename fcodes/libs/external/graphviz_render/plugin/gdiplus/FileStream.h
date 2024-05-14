/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#pragma once

#include <stdio.h>
#include <sys/stat.h>
#include <objbase.h>
#include <string>

class FileStream : public IStream
{
public:
	static IStream *Create(const char *name, FILE *file);

	/* IUnknown methods */

	HRESULT STDMETHODCALLTYPE QueryInterface(
		REFIID riid,
		void **ppvObject) override;

	ULONG STDMETHODCALLTYPE AddRef() override;

	ULONG STDMETHODCALLTYPE Release() override;

 	/* ISequentialStream methods */

	HRESULT STDMETHODCALLTYPE Read(
		void *pv,
		ULONG cb,
		ULONG *pcbRead) override;

	HRESULT STDMETHODCALLTYPE Write(
		const void *pv,
		ULONG cb,
		ULONG *pcbWritten) override;

	/* IStream methods */

	HRESULT STDMETHODCALLTYPE Seek(
		LARGE_INTEGER dlibMove,
		DWORD dwOrigin,
		ULARGE_INTEGER *plibNewPosition) override;

	HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize) override;

	HRESULT STDMETHODCALLTYPE CopyTo(
		IStream *pstm,
		ULARGE_INTEGER cb,
		ULARGE_INTEGER *pcbRead,
 		ULARGE_INTEGER *pcbWritten) override;

 	HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags) override;

 	HRESULT STDMETHODCALLTYPE Revert() override;

	HRESULT STDMETHODCALLTYPE LockRegion(
        	ULARGE_INTEGER libOffset,
        	ULARGE_INTEGER cb,
        	DWORD dwLockType) override;

	HRESULT STDMETHODCALLTYPE UnlockRegion(
        	ULARGE_INTEGER libOffset,
        	ULARGE_INTEGER cb,
        	DWORD dwLockType) override;

	HRESULT STDMETHODCALLTYPE Stat(
        	STATSTG *pstatstg,
        	DWORD grfStatFlag) override;

	HRESULT STDMETHODCALLTYPE Clone(IStream **ppstm) override;

	virtual ~FileStream() = default;

private:
	FileStream(const char *name, FILE *file);

	static void UnixTimeToFileTime(time_t unixTime, FILETIME &fileTime);

	ULONG _ref;
	std::string _name;
	FILE *_file;

};
