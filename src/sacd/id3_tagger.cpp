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

#include "id3_tagger.h"

#include "../../lib/id3v2lib/include/id3v2lib.h"

#include <kodi/Filesystem.h>
#include <kodi/General.h>

bool id3_tagger_t::load_info(std::vector<uint8_t>& buffer, kodi::addon::AudioDecoderInfoTag& info)
{
  if (buffer.empty())
    return false;

  ID3v2_tag* tag = load_tag_with_buffer(reinterpret_cast<char*>(buffer.data()), buffer.size());
  if (!tag)
    return false;

  ID3v2_frame* frame;
  frame = tag_get_title(tag);
  if (frame)
  {
    ID3v2_frame_text_content* content = parse_text_frame_content(frame);
    if (content && content->data)
      info.SetTitle(content->data);
  }
  frame = tag_get_artist(tag);
  if (frame)
  {
    ID3v2_frame_text_content* content = parse_text_frame_content(frame);
    if (content && content->data)
      info.SetArtist(content->data);
  }
  frame = tag_get_album(tag);
  if (frame)
  {
    ID3v2_frame_text_content* content = parse_text_frame_content(frame);
    if (content && content->data)
      info.SetAlbum(content->data);
  }
  frame = tag_get_album_artist(tag);
  if (frame)
  {
    ID3v2_frame_text_content* content = parse_text_frame_content(frame);
    if (content && content->data)
      info.SetAlbumArtist(content->data);
  }
  frame = tag_get_genre(tag);
  if (frame)
  {
    ID3v2_frame_text_content* content = parse_text_frame_content(frame);
    if (content && content->data)
      info.SetGenre(content->data);
  }
  frame = tag_get_track(tag);
  if (frame)
  {
    ID3v2_frame_text_content* content = parse_text_frame_content(frame);
    if (content && content->data)
      info.SetTrack(atoi(content->data));
  }
  frame = tag_get_year(tag);
  if (frame)
  {
    ID3v2_frame_text_content* content = parse_text_frame_content(frame);
    if (content && content->data)
      info.SetReleaseDate(content->data);
  }
  frame = tag_get_comment(tag);
  if (frame)
  {
    ID3v2_frame_comment_content* content = parse_comment_frame_content(frame);
    if (content && content->text)
    {
      if (content->text->data)
        info.SetComment(content->text->data);
      else if (content->short_description)
        info.SetComment(content->short_description);
    }
  }
  frame = tag_get_disc_number(tag);
  if (frame)
  {
    ID3v2_frame_text_content* content = parse_text_frame_content(frame);
    if (content && content->data)
      info.SetDisc(atoi(content->data));
  }
  /* TODO Implement support about composer and album cover
  frame = tag_get_composer(tag);
  if (frame)
  {
    ID3v2_frame_text_content* content = parse_text_frame_content(frame);
    if (content && content->data)
      info.SetComposer(content->data);
  }
  frame = tag_get_album_cover(tag);
  if (frame)
  {
    ID3v2_frame_apic_content* content = parse_apic_frame_content(frame);
    if (content && content->data)
    {

    }
  }*/

  free_id3_tag(tag);

  return true;
}

id3_tagger_t::iterator id3_tagger_t::begin()
{
  return iterator(tagstore, 0);
}

id3_tagger_t::iterator id3_tagger_t::end()
{
  return iterator(tagstore, tagstore.size());
}

size_t id3_tagger_t::get_count()
{
  return tagstore.size();
}

std::vector<id3_tags_t>& id3_tagger_t::get_info()
{
  return tagstore;
}

bool id3_tagger_t::is_single_track()
{
  return single_track;
}

void id3_tagger_t::set_single_track(bool is_single)
{
  single_track = is_single;
}

void id3_tagger_t::append(const id3_tags_t& tags)
{
  tagstore.push_back(tags);
  if (tags.id == -1)
  {
    update_tags(tagstore.size() - 1);
  }
}

void id3_tagger_t::remove_all()
{
  tagstore.clear();
}

bool id3_tagger_t::get_info(size_t track_number, kodi::addon::AudioDecoderInfoTag& info)
{
  auto ok = false;
  for (size_t track_index = 0; track_index < tagstore.size(); track_index++)
  {
    if (track_number == tagstore[track_index].id || single_track)
    {
      ok = load_info(track_index, info);
      break;
    }
  }
  return ok;
}

void id3_tagger_t::update_tags()
{
  for (size_t track_index = 0; track_index < tagstore.size(); track_index++)
  {
    update_tags(track_index);
  }
}

bool id3_tagger_t::load_info(size_t track_index, kodi::addon::AudioDecoderInfoTag& info)
{
  auto ok = false;
  if (track_index < tagstore.size())
  {
    ok = load_info(tagstore[track_index].value, info);
  }
  return ok;
}

void id3_tagger_t::update_tags(size_t track_index)
{
  if (track_index < tagstore.size())
  {
    kodi::addon::AudioDecoderInfoTag info;
    if (load_info(track_index, info))
    {
      tagstore[track_index].id = info.GetTrack();
    }
  }
}

void id3_tagger_t::get_albumart(size_t albumart_id, std::vector<uint8_t>& albumart_data)
{
}
