// This file is part of Micropolis-SDL2PP
// Micropolis-SDL2PP is based on Micropolis
//
// Copyright © 2022 Leeor Dicker
//
// Portions Copyright © 1989-2007 Electronic Arts Inc.
//
// Micropolis-SDL2PP is free software; you can redistribute it and/or modify
// it under the terms of the GNU GPLv3, with additional terms. See the README
// file, included in this distribution, for details.
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <string>

class FileIo
{
public:
	FileIo() = delete;
	FileIo(const FileIo&) = delete;
	const FileIo& operator=(const FileIo&) = delete;

	FileIo(SDL_Window& window);
	~FileIo() = default;

	const std::string& savePath() const { return mSavePath; }
	const std::string& fileName() const { return mFileName; }
	const std::string fullPath() const { return mSavePath + "\\" + mFileName; }

	bool pickSaveFile();
	bool pickOpenFile();

	bool filePicked() const;

private:
	enum class FileOperation { Open, Save };

	bool showFileDialog(FileOperation);
	void extractFileName();

	SDL_Window& mWindow;
	SDL_SysWMinfo mWmInfo{};

	std::string mSavePath;
	std::wstring mSavePathW;

	std::string mFileName;
	std::wstring mFileNameW;
};
