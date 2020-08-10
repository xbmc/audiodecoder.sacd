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

#include "sacd_media.h"

#include "scarletbook.h"

sacd_media_disc_t::~sacd_media_disc_t()
{
  close();
}

bool sacd_media_disc_t::open(const std::string& path, bool write)
{
  // TODO implement way to use real SACD disk's
  return false;
}

bool sacd_media_disc_t::close()
{
  // TODO implement way to use real SACD disk's
  return true;
}

bool sacd_media_disc_t::can_seek()
{
  return true;
}

bool sacd_media_disc_t::seek(int64_t position, int mode)
{
  switch (mode)
  {
    case SEEK_SET:
      file_position = position;
      break;
    case SEEK_CUR:
      file_position += position;
      break;
    case SEEK_END:
      file_position = get_size() - position;
      break;
  }
  return true;
}

std::shared_ptr<kodi::vfs::CFile> sacd_media_disc_t::get_handle()
{
  return std::shared_ptr<kodi::vfs::CFile>();
}

int64_t sacd_media_disc_t::get_position()
{
  return file_position;
}

int64_t sacd_media_disc_t::get_size()
{
  // TODO implement way to use real SACD disk's
  return -1;
}

kodi::vfs::FileStatus sacd_media_disc_t::get_stats()
{
  kodi::vfs::FileStatus filestats;
  // TODO implement way to use real SACD disk's
  return filestats;
}

size_t sacd_media_disc_t::read(void* data, size_t size)
{
  // TODO implement way to use real SACD disk's
  bool bReadOk = false;
  return bReadOk ? size : 0;
}

size_t sacd_media_disc_t::write(const void* data, size_t size)
{
  return 0;
}

int64_t sacd_media_disc_t::skip(int64_t bytes)
{
  file_position += bytes;
  return file_position;
}

void sacd_media_disc_t::truncate(int64_t position)
{
}

void sacd_media_disc_t::on_idle()
{
}

//------------------------------------------------------------------------------

sacd_media_file_t::sacd_media_file_t()
{
}

sacd_media_file_t::~sacd_media_file_t()
{
  close();
}

bool sacd_media_file_t::open(const std::string& path, bool write)
{
  m_path = path;
  std::shared_ptr<kodi::vfs::CFile> file = std::make_shared<kodi::vfs::CFile>();
  if (!write)
  {
    if (!file->OpenFile(path))
      return false;
  }
  else
  {
    if (!file->OpenFileForWrite(path))
      return false;
  }

  media_file = file;
  return true;
}

bool sacd_media_file_t::close()
{
  media_file.reset();
  return media_file.use_count() == 0;
}

bool sacd_media_file_t::can_seek()
{
  return media_file->IoControlGetSeekPossible();
}

bool sacd_media_file_t::seek(int64_t position, int mode)
{
  media_file->Seek(position, mode);
  return true;
}

std::shared_ptr<kodi::vfs::CFile> sacd_media_file_t::get_handle()
{
  return media_file;
}

int64_t sacd_media_file_t::get_position()
{
  return media_file->GetPosition();
}

int64_t sacd_media_file_t::get_size()
{
  return media_file->GetLength();
}

kodi::vfs::FileStatus sacd_media_file_t::get_stats()
{
  kodi::vfs::FileStatus stats;
  kodi::vfs::StatFile(m_path, stats);
  return stats;
}

size_t sacd_media_file_t::read(void* data, size_t size)
{
  return media_file->Read(data, size);
}

size_t sacd_media_file_t::write(const void* data, size_t size)
{
  media_file->Write(data, size);
  return size;
}

int64_t sacd_media_file_t::skip(int64_t bytes)
{
  return media_file->Seek(bytes, SEEK_CUR);
}

void sacd_media_file_t::truncate(int64_t position)
{
  media_file->Truncate(position);
}

void sacd_media_file_t::on_idle()
{
}
