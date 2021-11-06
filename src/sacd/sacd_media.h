/*
 *  Copyright (C) 2020 Team Kodi <https://kodi.tv>
 *  Copyright (c) 2011-2019 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

/*
 * Code taken SACD Decoder plugin (from foo_input_sacd) by Team Kodi.
 * Original author Maxim V.Anisiutkin.
 */

#pragma once

#include <kodi/Filesystem.h>
#include <memory>
#include <stdint.h>

class ATTR_DLL_LOCAL sacd_media_t
{
public:
  virtual ~sacd_media_t(){};

  virtual bool open(const std::string& path, bool write) = 0;
  virtual bool close() = 0;
  virtual bool can_seek() = 0;
  virtual bool seek(int64_t position, int mode = SEEK_SET) = 0;
  virtual std::shared_ptr<kodi::vfs::CFile> get_handle() = 0;
  virtual int64_t get_position() = 0;
  virtual int64_t get_size() = 0;
  virtual kodi::vfs::FileStatus get_stats() = 0;
  virtual size_t read(void* data, size_t size) = 0;
  virtual size_t write(const void* data, size_t size) = 0;
  virtual int64_t skip(int64_t bytes) = 0;
  virtual void truncate(int64_t position) = 0;
  virtual void on_idle() = 0;
};

class ATTR_DLL_LOCAL sacd_media_disc_t : public sacd_media_t
{
public:
  sacd_media_disc_t() = default;
  virtual ~sacd_media_disc_t();

  bool open(const std::string& path, bool write) override;
  bool close() override;
  bool can_seek() override;
  bool seek(int64_t position, int mode = SEEK_SET) override;
  std::shared_ptr<kodi::vfs::CFile> get_handle() override;
  int64_t get_position() override;
  int64_t get_size() override;
  kodi::vfs::FileStatus get_stats() override;
  size_t read(void* data, size_t size) override;
  size_t write(const void* data, size_t size) override;
  int64_t skip(int64_t bytes) override;
  void truncate(int64_t position) override;
  void on_idle() override;

private:
  //   HANDLE  media_disc = INVALID_HANDLE_VALUE;
  int64_t file_position = -1;
};

class ATTR_DLL_LOCAL sacd_media_file_t : public sacd_media_t
{
public:
  sacd_media_file_t();
  ~sacd_media_file_t();

  bool open(const std::string& path, bool write) override;
  bool close() override;
  bool can_seek() override;
  bool seek(int64_t position, int mode = SEEK_SET) override;
  std::shared_ptr<kodi::vfs::CFile> get_handle() override;
  int64_t get_position() override;
  int64_t get_size() override;
  kodi::vfs::FileStatus get_stats() override;
  size_t read(void* data, size_t size) override;
  size_t write(const void* data, size_t size) override;
  int64_t skip(int64_t bytes) override;
  void truncate(int64_t position) override;
  void on_idle() override;

private:
  std::shared_ptr<kodi::vfs::CFile> media_file;
  std::string m_path;
};
