/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "utils/RegExp.h"
#include "utils/URIUtils.h"

#include <string>

#include <gtest/gtest.h>

namespace
{

// A disc's own file and the bluray:// path of one of its playlists answer the same.
struct DiscCase
{
  const char* file;
  const char* playlist;
  const char* expected;
};

struct PathCase
{
  const char* path;
  const char* expected;
};

const PathCase RemoveDiscPathCases[] = {
    {"smb://somepath/Movie/BDMV/index.bdmv", "smb://somepath/Movie/"},
    {"smb://somepath/Movie/VIDEO_TS/VIDEO_TS.IFO", "smb://somepath/Movie/"},
    // A disc file directly in the movie folder has no trailing slash
    {"smb://somepath/Movie/VIDEO_TS.IFO", "smb://somepath/Movie"},
    {"smb://somepath/Movie/movie.mkv", ""},
};

const DiscCase GetDiscBaseCases[] = {
    {"/somepath/path/BDMV/index.bdmv", "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     "/somepath/path/"},
    {"/somepath/path/movie.iso",
     "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     "/somepath/path/movie.iso"},
    {"D:\\Movies\\BDMV\\index.bdmv", "bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
     "D:\\Movies\\"},
    {"D:\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     "D:\\Movies\\movie.iso"},
    {"\\\\Server\\Movies\\BDMV\\index.bdmv",
     "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls", "\\\\Server\\Movies\\"},
    {"\\\\Server\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     "\\\\Server\\Movies\\movie.iso"},
    {"smb://somepath/path/BDMV/index.bdmv",
     "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls", "smb://somepath/path/"},
    {"smb://somepath/path/movie.iso",
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "smb://somepath/path/movie.iso"},
};

const DiscCase GetDiscBasePathCases[] = {
    {"/somepath/path/BDMV/index.bdmv", "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     "/somepath/path/"},
    {"/somepath/path/movie.iso",
     "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     "/somepath/path/"},
    {"/somepath/path/movie.mkv", nullptr, "/somepath/path/"},
    {"D:\\Movies\\BDMV\\index.bdmv", "bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
     "D:\\Movies\\"},
    {"D:\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     "D:\\Movies\\"},
    {"\\\\Server\\Movies\\BDMV\\index.bdmv",
     "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls", "\\\\Server\\Movies\\"},
    {"\\\\Server\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     "\\\\Server\\Movies\\"},
    {"smb://somepath/path/BDMV/index.bdmv",
     "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls", "smb://somepath/path/"},
    {"smb://somepath/path/movie.iso",
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "smb://somepath/path/"},
};

const PathCase GetDiscFileCases[] = {
    {"bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls", "/somepath/path/BDMV/index.bdmv"},
    {"bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     "/somepath/path/movie.iso"},
    {"bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls", "D:\\Movies\\BDMV\\index.bdmv"},
    {"bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     "D:\\Movies\\movie.iso"},
    {"bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
     "\\\\Server\\Movies\\BDMV\\index.bdmv"},
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     "\\\\Server\\Movies\\movie.iso"},
    {"bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     "smb://somepath/path/BDMV/index.bdmv"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "smb://somepath/path/movie.iso"},
};

const PathCase GetDiscUnderlyingFileCases[] = {
    {"bluray://%2fsomepath%2fpath%2f/file.ext", "/somepath/path/file.ext"},
    {"bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/file.ext",
     "udf://%2fsomepath%2fpath%2fmovie.iso/file.ext"},
    {"bluray://D%3a%5cMovies%5c/file.ext", "D:\\Movies\\file.ext"},
    {"bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/file.ext",
     "udf://D%3a%5cMovies%5cmovie.iso/file.ext"},
    {"bluray://%5c%5cServer%5cMovies%5c/file.ext", "\\\\Server\\Movies\\file.ext"},
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/file.ext",
     "udf://%5c%5cServer%5cMovies%5cmovie.iso/file.ext"},
    {"bluray://smb%3a%2f%2fsomepath%2fpath%2f/file.ext", "smb://somepath/path/file.ext"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/file.ext",
     "udf://smb%3a%2f%2fsomepath%2fpath%2fmovie.iso/file.ext"},
};

struct BlurayTitlesCase
{
  const char* file;
  const char* playlist;
  URIUtils::GetAllTitles titles;
  URIUtils::AllTitlesOptions options;
  const char* expected;
};

using Titles = URIUtils::GetAllTitles;
using Options = URIUtils::AllTitlesOptions;

const BlurayTitlesCase GetBlurayTitlesPathCases[] = {
    {"/somepath/path/BDMV/index.bdmv", "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::LONG, Options::MOVIES, "bluray://%2fsomepath%2fpath%2f/root/titles"},
    {"/somepath/path/movie.iso",
     "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::LONG, Options::MOVIES,
     "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/root/titles"},
    {"D:\\Movies\\BDMV\\index.bdmv", "bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
     Titles::LONG, Options::MOVIES, "bluray://D%3a%5cMovies%5c/root/titles"},
    {"D:\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::LONG, Options::MOVIES,
     "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/root/titles"},
    {"\\\\Server\\Movies\\BDMV\\index.bdmv",
     "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls", Titles::LONG, Options::MOVIES,
     "bluray://%5c%5cServer%5cMovies%5c/root/titles"},
    {"\\\\Server\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::LONG, Options::MOVIES,
     "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/root/titles"},
    {"smb://somepath/path/BDMV/index.bdmv",
     "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls", Titles::LONG,
     Options::MOVIES, "bluray://smb%3a%2f%2fsomepath%2fpath%2f/root/titles"},
    {"smb://somepath/path/movie.iso",
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     Titles::LONG, Options::MOVIES,
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/root/titles"},

    {"/somepath/path/BDMV/index.bdmv", "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::ALL, Options::MOVIES, "bluray://%2fsomepath%2fpath%2f/root/titles/all"},
    {"/somepath/path/movie.iso",
     "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::ALL, Options::MOVIES,
     "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/root/titles/all"},
    {"D:\\Movies\\BDMV\\index.bdmv", "bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
     Titles::ALL, Options::MOVIES, "bluray://D%3a%5cMovies%5c/root/titles/all"},
    {"D:\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::ALL, Options::MOVIES,
     "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/root/titles/all"},
    {"\\\\Server\\Movies\\BDMV\\index.bdmv",
     "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls", Titles::ALL, Options::MOVIES,
     "bluray://%5c%5cServer%5cMovies%5c/root/titles/all"},
    {"\\\\Server\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::ALL, Options::MOVIES,
     "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/root/titles/all"},
    {"smb://somepath/path/BDMV/index.bdmv",
     "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls", Titles::ALL,
     Options::MOVIES, "bluray://smb%3a%2f%2fsomepath%2fpath%2f/root/titles/all"},
    {"smb://somepath/path/movie.iso",
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     Titles::ALL, Options::MOVIES,
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/root/titles/all"},

    {"/somepath/path/BDMV/index.bdmv", "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::LONG, Options::EPISODES, "bluray://%2fsomepath%2fpath%2f/root/titles/episodes"},
    {"/somepath/path/movie.iso",
     "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::LONG, Options::EPISODES,
     "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/root/titles/episodes"},
    {"D:\\Movies\\BDMV\\index.bdmv", "bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
     Titles::LONG, Options::EPISODES, "bluray://D%3a%5cMovies%5c/root/titles/episodes"},
    {"D:\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::LONG, Options::EPISODES,
     "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/root/titles/episodes"},
    {"\\\\Server\\Movies\\BDMV\\index.bdmv",
     "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls", Titles::LONG, Options::EPISODES,
     "bluray://%5c%5cServer%5cMovies%5c/root/titles/episodes"},
    {"\\\\Server\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::LONG, Options::EPISODES,
     "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/root/titles/episodes"},
    {"smb://somepath/path/BDMV/index.bdmv",
     "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls", Titles::LONG,
     Options::EPISODES, "bluray://smb%3a%2f%2fsomepath%2fpath%2f/root/titles/episodes"},
    {"smb://somepath/path/movie.iso",
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     Titles::LONG, Options::EPISODES,
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/root/titles/"
     "episodes"},

    {"/somepath/path/BDMV/index.bdmv", "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::ALL, Options::EPISODES, "bluray://%2fsomepath%2fpath%2f/root/titles/episodes/all"},
    {"/somepath/path/movie.iso",
     "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::ALL, Options::EPISODES,
     "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/root/titles/episodes/all"},
    {"D:\\Movies\\BDMV\\index.bdmv", "bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
     Titles::ALL, Options::EPISODES, "bluray://D%3a%5cMovies%5c/root/titles/episodes/all"},
    {"D:\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::ALL, Options::EPISODES,
     "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/root/titles/episodes/all"},
    {"\\\\Server\\Movies\\BDMV\\index.bdmv",
     "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls", Titles::ALL, Options::EPISODES,
     "bluray://%5c%5cServer%5cMovies%5c/root/titles/episodes/all"},
    {"\\\\Server\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     Titles::ALL, Options::EPISODES,
     "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/root/titles/episodes/all"},
    {"smb://somepath/path/BDMV/index.bdmv",
     "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls", Titles::ALL,
     Options::EPISODES, "bluray://smb%3a%2f%2fsomepath%2fpath%2f/root/titles/episodes/all"},
    {"smb://somepath/path/movie.iso",
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     Titles::ALL, Options::EPISODES,
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/root/titles/"
     "episodes/all"},
};

const DiscCase GetBlurayEpisodePathCases[] = {
    {"/somepath/path/BDMV/index.bdmv", "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     "bluray://%2fsomepath%2fpath%2f/root/episode/3/4"},
    {"/somepath/path/movie.iso",
     "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/root/episode/3/4"},
    {"D:\\Movies\\BDMV\\index.bdmv", "bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
     "bluray://D%3a%5cMovies%5c/root/episode/3/4"},
    {"D:\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/root/episode/3/4"},
    {"\\\\Server\\Movies\\BDMV\\index.bdmv",
     "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
     "bluray://%5c%5cServer%5cMovies%5c/root/episode/3/4"},
    {"\\\\Server\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/root/episode/3/4"},
    {"smb://somepath/path/BDMV/index.bdmv",
     "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     "bluray://smb%3a%2f%2fsomepath%2fpath%2f/root/episode/3/4"},
    {"smb://somepath/path/movie.iso",
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/root/episode/3/4"},
};

const DiscCase GetBlurayExtraTitlesPathCases[] = {
    {"/somepath/path/BDMV/index.bdmv", "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     "bluray://%2fsomepath%2fpath%2f/root/extras/titles"},
};

const DiscCase GetBlurayExtrasPathCases[] = {
    {"/somepath/path/BDMV/index.bdmv", "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     "bluray://%2fsomepath%2fpath%2f/root/extras"},
    {"D:\\Movies\\BDMV\\index.bdmv", nullptr, "bluray://D%3a%5cMovies%5c/root/extras"},
    {"smb://somepath/path/movie.iso",
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/root/extras"},
};

struct BlurayPlaylistCase
{
  const char* file;
  const char* playlistFolder;
  const char* playlist;
};

const BlurayPlaylistCase GetBlurayPlaylistPathCases[] = {
    {"/somepath/path/BDMV/index.bdmv", "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/",
     "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls"},
    {"/somepath/path/movie.iso",
     "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/",
     "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls"},
    {"D:\\Movies\\BDMV\\index.bdmv", "bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/",
     "bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls"},
    {"D:\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/",
     "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls"},
    {"\\\\Server\\Movies\\BDMV\\index.bdmv", "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/",
     "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls"},
    {"\\\\Server\\Movies\\movie.iso",
     "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/",
     "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls"},
    {"smb://somepath/path/BDMV/index.bdmv",
     "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/",
     "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls"},
    {"smb://somepath/path/movie.iso",
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/",
     "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls"},
};

struct PartNumberCase
{
  const char* title;
  int position;
};

const PartNumberCase PartNumberCases[] = {
    {"Movie (Disc 1)", 5},
    {"Movie-(Disk 100)", 5},
    {"Movie", -1},
};

} // namespace

TEST(TestURIUtilsDiscPaths, RemoveDiscPath)
{
  for (const auto& [path, expected] : RemoveDiscPathCases)
  {
    EXPECT_EQ(expected, URIUtils::RemoveDiscPath(path)) << path;
  }
}

TEST(TestURIUtilsDiscPaths, GetDiscBase)
{
  for (const auto& [file, playlist, expected] : GetDiscBaseCases)
  {
    EXPECT_EQ(expected, URIUtils::GetDiscBase(file)) << file;
    EXPECT_EQ(expected, URIUtils::GetDiscBase(playlist)) << playlist;
  }
}

TEST(TestURIUtilsDiscPaths, GetDiscBasePath)
{
  for (const auto& [file, playlist, expected] : GetDiscBasePathCases)
  {
    EXPECT_EQ(expected, URIUtils::GetDiscBasePath(file)) << file;
    if (playlist)
    {
      EXPECT_EQ(expected, URIUtils::GetDiscBasePath(playlist)) << playlist;
    }
  }
}

TEST(TestURIUtilsDiscPaths, GetDiscFile)
{
  for (const auto& [playlist, expected] : GetDiscFileCases)
  {
    EXPECT_EQ(expected, URIUtils::GetDiscFile(playlist)) << playlist;
  }
}

TEST(TestURIUtilsDiscPaths, GetDiscUnderlyingFile)
{
  for (const auto& [path, expected] : GetDiscUnderlyingFileCases)
  {
    EXPECT_EQ(expected, URIUtils::GetDiscUnderlyingFile(CURL{path})) << path;
  }
}

TEST(TestURIUtilsDiscPaths, GetBlurayTitlesPath)
{
  for (const auto& [file, playlist, titles, options, expected] : GetBlurayTitlesPathCases)
  {
    EXPECT_EQ(expected, URIUtils::GetBlurayTitlesPath(file, titles, options)) << file;
    EXPECT_EQ(expected, URIUtils::GetBlurayTitlesPath(playlist, titles, options)) << playlist;
  }
}

TEST(TestURIUtilsDiscPaths, GetBlurayEpisodePath)
{
  for (const auto& [file, playlist, expected] : GetBlurayEpisodePathCases)
  {
    EXPECT_EQ(expected, URIUtils::GetBlurayEpisodePath(file, 3, 4)) << file;
    EXPECT_EQ(expected, URIUtils::GetBlurayEpisodePath(playlist, 3, 4)) << playlist;
  }
}

TEST(TestURIUtilsDiscPaths, GetBlurayExtraTitlesPath)
{
  for (const auto& [file, playlist, expected] : GetBlurayExtraTitlesPathCases)
  {
    EXPECT_EQ(expected, URIUtils::GetBlurayExtraTitlesPath(file)) << file;
    EXPECT_EQ(expected, URIUtils::GetBlurayExtraTitlesPath(playlist)) << playlist;
  }
}

TEST(TestURIUtilsDiscPaths, GetBlurayExtrasPath)
{
  for (const auto& [file, playlist, expected] : GetBlurayExtrasPathCases)
  {
    EXPECT_EQ(expected, URIUtils::GetBlurayExtrasPath(file)) << file;
    if (playlist)
    {
      EXPECT_EQ(expected, URIUtils::GetBlurayExtrasPath(playlist)) << playlist;
    }
  }
}

TEST(TestURIUtilsDiscPaths, GetBlurayPlaylistPath)
{
  for (const auto& [file, playlistFolder, playlist] : GetBlurayPlaylistPathCases)
  {
    EXPECT_EQ(playlistFolder, URIUtils::GetBlurayPlaylistPath(file)) << file;
    EXPECT_EQ(playlist, URIUtils::GetBlurayPlaylistPath(file, 800)) << file;
  }
}

TEST(TestURIUtilsDiscPaths, GetBlurayPlaylistFromPath)
{
  for (const auto& [file, playlist, expected] : GetDiscBaseCases)
  {
    EXPECT_EQ(800, URIUtils::GetBlurayPlaylistFromPath(playlist)) << playlist;
  }
}

TEST(TestURIUtilsDiscPaths, RemovePartNumberFromTitle)
{
  const std::string r{URIUtils::GetTitleTrailingPartNumberRegex()};
  CRegExp regex{true, CRegExp::autoUtf8, r.c_str()};

  for (const auto& [title, position] : PartNumberCases)
  {
    EXPECT_EQ(position, regex.RegFind(title)) << title;
  }
}
