/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../DynamicDatabase.h"
#include "dbwrappers/dataset.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/Directory.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/IDBInfoTag.h"
#include "utils/Variant.h"
#include "utils/Vector.h"
#include "music/MusicDatabase.h"
#include "FileItem.h"

#include "gtest/gtest.h"

#include <vector>
#include <string>

// MySQL connection info. If the database can't connect, MySQL tests will be skipped.
#define MYSQLHOST "127.0.0.1"
#define MYSQLPORT "3306"
#define MYSQLUSER "root"
#define MYSQLPASS "root"

using namespace XFILE;
using namespace std;


// Test strategy: We implement the virtual database, CDynamicDatabase, and the
// abstract object it operates on, IDBInfoTag. Lawn gnomes are suited for this
// task because they live in a garden (1:N), travel to different places (N:N),
// and have a name and a number of little gnome friends (1:1 indices of types
// TEXT and INTEGER).


///////////////////////////////////////////////////////////////////////////////
// Test abstract info tag implementation: CLawnGnome
class CLawnGnome : public IDBInfoTag
{
public:
  string         m_name;
  string         m_gardenName;
  int            m_littleGnomeFriends;
  vector<string> m_placesTraveled;

  CLawnGnome() { }
  CLawnGnome(string name, string gardenName, int littleGnomeFriends)
    : m_name(name), m_gardenName(gardenName), m_littleGnomeFriends(littleGnomeFriends)
  {
  }

  void TravelTo(const string &strPlace) { m_placesTraveled.push_back(strPlace); }

  virtual void Deserialize(const CVariant& value)
  {
    m_name               = value["name"].asString();
    m_gardenName         = value["garden"].asString();
    m_littleGnomeFriends = (int)value["littlegnomefriends"].asInteger();
    CVariant arr         = value["placestraveled"];
    for (CVariant::const_iterator_array it = arr.begin_array(); it != arr.end_array(); it++)
      m_placesTraveled.push_back(it->asString());
  }

  virtual void Serialize(CVariant& value) const
  {
    value["name"]                = m_name;
    value["garden"]              = m_gardenName;
    value["littlegnomefriends"]  = m_littleGnomeFriends;
    for (unsigned int i = 0; i < m_placesTraveled.size(); i++)
      value["placestraveled"].push_back(m_placesTraveled[i]);
  }
};


///////////////////////////////////////////////////////////////////////////////
// Test virtual database implementation: CGnomeDatabase
class CGnomeDatabase : public CDynamicDatabase
{
public:
  // Allow sqlite/mysql to be specified on construction
  CGnomeDatabase() : CDynamicDatabase("gnome")
  {
    BeginDeclarations();
    DeclareIndex("name", "VARCHAR(512)");
    DeclareOneToMany("garden", "VARCHAR(512)", true);
    DeclareIndex("littlegnomefriends", "INTEGER");
    DeclareManyToMany("placestraveled", "TEXT", false);
  }
  virtual ~CGnomeDatabase() { }
  virtual int GetMinVersion() const { return 1; }
  virtual const char *GetBaseDBName() const { return "MyGnomes"; }
  virtual bool CreateTables()
  {
    try
    {
      BeginTransaction();
      if (!CDynamicDatabase::CreateTables())
        return false;
      CommitTransaction();
      return true;
    }
    catch (dbiplus::DbErrors)
    {
      RollbackTransaction();
    }
    return false;
  }
  virtual bool IsValid(const CVariant &object) const
  {
    return !object["name"].isNull() && !object["garden"].isNull() &&
        object["name"].asString().length() != 0 && object["garden"].asString().length() != 0;
  }
  virtual bool Exists(const CVariant &object, int &idObject) throw(dbiplus::DbErrors)
  {
    if (!IsValid(object))
      return false;
    CStdString strSQL = PrepareSQL(
      "SELECT id%s "
      "FROM %s JOIN garden ON garden.idgarden=%s.idgarden "
      "WHERE name='%s' AND garden='%s'",
      Describe(), Describe(), Describe(), object["name"].asString().c_str(), object["garden"].asString().c_str()
    );
    if (m_pDS->query(strSQL.c_str()))
    {
      bool bFound = false;
      if (m_pDS->num_rows() != 0)
      {
        idObject = m_pDS->fv(0).get_asInt();
        bFound = true;
      }
      m_pDS->close();
      return bFound;
    }
    return false;
  }
  virtual CFileItem *CreateFileItem(const std::string &json, int id) const
  {
    CLawnGnome gnome;
    DeserializeJSON(json, id, &gnome);
    CFileItem *item = new CFileItem(gnome.m_name);
    item->SetPath(gnome.m_gardenName);
    return item;
  }
  bool DropGnomeDatabase()
  {
    if (m_sqlite) return false; // just in case
    if (m_pDB.get() == NULL) return false;
    if (m_pDS.get() == NULL) return false;
    try
    {
      m_pDS->exec("DROP DATABASE IF EXISTS MyGnomes1");
      return true;
    }
    catch (...) { }
    return false;
  }
  // Remap to public
  bool Update(const DatabaseSettings &dbs) { return CDatabase::Update(dbs); }
  bool GetAllTables(vector<CStdString> &tableList) { return CDatabase::GetAllTables(tableList); }
  bool GetAllIndices(vector<CStdString> &indexList) { return CDatabase::GetAllIndices(indexList); }
  void AddIndex(const char *column, const char *type) { CDynamicDatabase::AddIndex(column, type); }
  void AddOneToMany(const char *column, const char *type, bool index) { CDynamicDatabase::AddOneToMany(column, type, index); }
  void AddManyToMany(const char *column, const char *type, bool index) { CDynamicDatabase::AddManyToMany(column, type, index); }
  void DropIndex(const char *column) { CDynamicDatabase::DropIndex(column); }
  void DropOneToMany(const char *column) { CDynamicDatabase::DropOneToMany(column); }
  void DropManyToMany(const char *column) { CDynamicDatabase::DropManyToMany(column); }
};


///////////////////////////////////////////////////////////////////////////////
// Test procedures
void runTests(CGnomeDatabase &db);
bool has(const vector<CStdString> &haystack, const CStdString &needle)
{
  return find(haystack.begin(), haystack.end(), needle) != haystack.end();
}

TEST(TestDynamicDatabase, DynamicDatabaseSQLite)
{
  // Create the Database folder if it wasn't done in the setup process
  if (!CDirectory::Exists(g_settings.GetDatabaseFolder()))
    CDirectory::Create(g_settings.GetDatabaseFolder());

  // Calling Update creates the database if it doesn't exist
  CGnomeDatabase db;
  EXPECT_TRUE(db.Update(DatabaseSettings()));

  EXPECT_NO_FATAL_FAILURE(runTests(db));

  CDirectory::Remove(g_settings.GetDatabaseFolder());
}

TEST(TestDynamicDatabase, DynamicDatabaseMySQL)
{
  DatabaseSettings dbs;
  dbs.type = "mysql";
  dbs.host = MYSQLHOST;
  dbs.port = MYSQLPORT;
  dbs.user = MYSQLUSER;
  dbs.pass = MYSQLPASS;
  dbs.name = "MyGnomes";

  CGnomeDatabase db;

  // Only test MySQL if we have a valid connection
  if (!db.Update(dbs))
  {
    cout << "Can't connect to MySQL, skipping tests. Fix params in TestDynamicDatabase.cpp" << endl;
    return;
  }

  EXPECT_NO_FATAL_FAILURE(runTests(db));

  EXPECT_TRUE(db.DropGnomeDatabase());
}

void runTests(CGnomeDatabase &db)
{
  ASSERT_TRUE(db.IsOpen());

  // Set up our test data
  CLawnGnome Gnomeo        ("Gnomeo",         "Montague Garden",     4); // Venice, Zurich
  CLawnGnome Broomhilda    ("'\"\b\n\r\\n ",  "Ikea Garden",         4); // Zurich, Tokyo, Lisbon
  CLawnGnome Fruuuugenhagen("Fruuuugenhagen", "Ikea Garden",         1); // Belize, Tokyo, Lisbon
  CLawnGnome DrBrukunduke  ("Dr. Brukunduke", "Ikea Garden",         1); // Monte Carlo
  CLawnGnome Miserputin    ("Miserputin",     "Noodlepushin Garden", 4); // Lisbon, Georgia, Bangledash
  CLawnGnome Rekelpuke     ("Rekelpuke",      "RussiaIsCold Garden", 0);
  CLawnGnome Vuvelvive     ("Vuvelvive",      "",                    7); // Louisiana, Oregon, Vancouver, Wyoming
  CLawnGnome Zulu          ("Zulu",           "Awsomeitz",           6); // Space
  CLawnGnome Kirk          ("Kirk",           "Awsomeitz",           3);
  CLawnGnome Whitey        ("",               "Nookiedom",           0);

  Gnomeo.TravelTo("Venice");
  Gnomeo.TravelTo("Zurich");
  Broomhilda.TravelTo("Zurich");
  Broomhilda.TravelTo("Tokyo");
  Broomhilda.TravelTo("Lisbon");
  Fruuuugenhagen.TravelTo("Belize");
  Fruuuugenhagen.TravelTo("Tokyo");
  Fruuuugenhagen.TravelTo("Lisbon");
  DrBrukunduke.TravelTo("Monte Carlo");
  Miserputin.TravelTo("Lisbon");
  Miserputin.TravelTo("Georgia");
  Miserputin.TravelTo("Bangledash");
  Vuvelvive.TravelTo("Louisiana");
  Vuvelvive.TravelTo("Oregon");
  Vuvelvive.TravelTo("Vancouver");
  Vuvelvive.TravelTo("Wyoming");
  Zulu.TravelTo("Space");
  Kirk.TravelTo("Space");

  // Test database tables post-creation
  vector<CStdString> tables;
  EXPECT_TRUE(db.GetAllTables(tables));
  EXPECT_EQ(tables.size(), 5);
  EXPECT_TRUE(has(tables, "version"));
  EXPECT_TRUE(has(tables, "gnome"));
  EXPECT_TRUE(has(tables, "garden"));
  EXPECT_TRUE(has(tables, "placestraveled"));
  EXPECT_TRUE(has(tables, "gnomelinkplacestraveled"));

  // Test database indices post-creation
  vector<CStdString> indices;
  EXPECT_TRUE(db.GetAllIndices(indices));
  // MySQL doesn't currently create link1 or link2
  EXPECT_EQ(indices.size(), 8);
  EXPECT_TRUE(has(indices, "idx_gnome_name"));
  EXPECT_TRUE(has(indices, "idx_gnome_garden"));
  EXPECT_TRUE(has(indices, "idx_gnome_littlegnomefriends"));
  EXPECT_TRUE(has(indices, "idx_garden_garden"));
  EXPECT_TRUE(has(indices, "idx_gnomelinkplacestraveled_gnome"));
  EXPECT_TRUE(has(indices, "idx_gnomelinkplacestraveled_placestraveled"));
  EXPECT_TRUE(has(indices, "idx_gnomelinkplacestraveled_link1"));
  EXPECT_TRUE(has(indices, "idx_gnomelinkplacestraveled_link2"));

  // Try adding some data
  int id1, id2;
  EXPECT_EQ(db.AddObject(NULL), -1);
  EXPECT_NE(id1=db.AddObject(&Gnomeo), -1);
  EXPECT_EQ(db.AddObject(&Gnomeo, true), id1);
  EXPECT_EQ(db.AddObject(&Gnomeo, false), id1);
  EXPECT_NE(id2=db.AddObject(&Broomhilda), -1);
  EXPECT_EQ(db.AddObject(&Broomhilda, true), id2);
  EXPECT_EQ(db.AddObject(&Broomhilda, false), id2);
  EXPECT_NE(id1, id2);

  EXPECT_NE(db.AddObject(&Fruuuugenhagen), -1);
  EXPECT_NE(db.AddObject(&DrBrukunduke), -1);
  EXPECT_NE(db.AddObject(&Miserputin), -1);
  EXPECT_NE(db.AddObject(&Rekelpuke), -1);
  EXPECT_EQ(db.Count(), 6);
  EXPECT_EQ(db.Count("garden"), 4);
  EXPECT_EQ(db.Count("placestraveled"), 8);
  EXPECT_EQ(db.Count("doesntexist"), 0);

  // Test empty name and garden
  EXPECT_EQ(db.AddObject(&Vuvelvive), -1);
  EXPECT_EQ(db.AddObject(&Whitey), -1);
  EXPECT_EQ(db.Count(), 6);

  // Test data retrieval
  CLawnGnome BabyGnomeo, BabyBroomhilda, BabyFruuuugenhagen, Gnome1, Gnome2;
  EXPECT_TRUE(db.GetObjectByID(id1, &BabyGnomeo));
  EXPECT_EQ(BabyGnomeo.m_name, Gnomeo.m_name);
  EXPECT_EQ(BabyGnomeo.m_placesTraveled.size(), 2);
  EXPECT_TRUE(db.GetObjectByID(id2, &BabyBroomhilda));
  EXPECT_EQ(BabyBroomhilda.m_name, Broomhilda.m_name);
  EXPECT_TRUE(db.GetObjectByIndex("name", Fruuuugenhagen.m_name, &BabyFruuuugenhagen));
  EXPECT_EQ(Fruuuugenhagen.m_name, BabyFruuuugenhagen.m_name);
  EXPECT_TRUE(db.GetObjectByIndex("littlegnomefriends", 1, &Gnome1));
  EXPECT_TRUE(db.GetObjectByIndex("littlegnomefriends", "1", &Gnome2));
  EXPECT_EQ(Gnome1.m_name, Gnome2.m_name);
  EXPECT_FALSE(db.GetObjectByIndex("name", "", &BabyGnomeo));
  EXPECT_FALSE(db.GetObjectByIndex("name", "Auschwitzer", &BabyGnomeo));
  EXPECT_FALSE(db.GetObjectByIndex("unknown", "Flabernoun", &BabyGnomeo));
  EXPECT_FALSE(db.GetObjectByIndex("littlegnomefriends", 100, &BabyGnomeo));

  CFileItemList gardens, places, temp, gardensOnlyLisbon;
  EXPECT_TRUE(db.GetItemNav("garden", gardens, ""));
  EXPECT_EQ(gardens.Size(), 4);
  EXPECT_TRUE(db.GetItemNav("placestraveled", places, ""));
  EXPECT_EQ(places.Size(), 8);
  EXPECT_FALSE(db.GetItemNav("doesntexist", temp, ""));

  long idLisbon = -1;
  for (int i = 0; i < places.Size(); i++)
  {
    if (places[i]->GetLabel() == "Lisbon")
    {
      // Remove the slash
      CStdString place = places[i]->GetPath();
      idLisbon = atol(place.substr(0, place.length() - 1).c_str());
      break;
    }
  }
  EXPECT_NE(idLisbon, -1);
  int id = -1;
  EXPECT_TRUE(db.GetItemID("placestraveled", "Lisbon", id));
  EXPECT_NE(id, -1);
  EXPECT_EQ(idLisbon, id);
  id = -1;
  EXPECT_TRUE(db.GetItemID("garden", "Ikea Garden", id));
  EXPECT_NE(id, -1);
  EXPECT_FALSE(db.GetItemID("doesntexist", "value", id));

  // In the meantime, test GetItemByID()
  CVariant varLisbon;
  EXPECT_TRUE(db.GetItemByID("placestraveled", idLisbon, varLisbon));
  EXPECT_EQ(varLisbon.asString(), "Lisbon");
  EXPECT_FALSE(db.GetItemByID("garden", 99, varLisbon));
  EXPECT_FALSE(db.GetItemByID("doesntexist", idLisbon, varLisbon));

  map<string, long> predicates;
  predicates["placestraveled"] = idLisbon;
  EXPECT_TRUE(db.GetItemNav("garden", gardensOnlyLisbon, "", predicates));
  EXPECT_EQ(gardensOnlyLisbon.Size(), 2);
  EXPECT_TRUE(gardensOnlyLisbon[0]->GetLabel() == "Ikea Garden");
  EXPECT_TRUE(gardensOnlyLisbon[1]->GetLabel() == "Noodlepushin Garden");
  long idNoodlepushin = -1;
  for (int i = 0; i < gardensOnlyLisbon.Size(); i++)
  {
    if (gardensOnlyLisbon[i]->GetLabel() == "Noodlepushin Garden")
    {
      // Remove the slash
      CStdString garden = gardensOnlyLisbon[i]->GetPath();
      idNoodlepushin = atol(garden.substr(0, garden.length() - 1).c_str());
      break;
    }
  }
  EXPECT_NE(idNoodlepushin, -1);
  predicates["garden"] = idNoodlepushin;
  gardensOnlyLisbon.Clear();
  EXPECT_TRUE(db.GetItemNav("garden", gardensOnlyLisbon, "", predicates));
  EXPECT_EQ(gardensOnlyLisbon.Size(), 1);
  EXPECT_TRUE(gardensOnlyLisbon[0]->GetLabel() == "Noodlepushin Garden");
  
  // Test object nav
  CFileItemList objects;
  EXPECT_TRUE(db.GetObjectsNav(objects));
  EXPECT_EQ(objects.Size(), 6);
  objects.Clear();
  map<string, long> predicatesObj;
  predicatesObj["placestraveled"] = idLisbon;
  EXPECT_TRUE(db.GetObjectsNav(objects, predicatesObj));
  EXPECT_EQ(objects.Size(), 3);

  // Test deleting objects and check for orphans
  EXPECT_TRUE(db.DeleteObject(id1, false));
  EXPECT_EQ(db.Count("garden"), 4);
  EXPECT_EQ(db.Count("placestraveled"), 8);
  EXPECT_TRUE(db.DeleteObject(id1));
  EXPECT_NE(id1=db.AddObject(&Gnomeo), -1); // re-add, get the different ID
  EXPECT_EQ(db.Count("garden"), 4);
  EXPECT_EQ(db.Count("placestraveled"), 8);
  EXPECT_TRUE(db.DeleteObject(id1, true));
  EXPECT_EQ(db.Count("garden"), 3); // minus Montague Garden
  EXPECT_EQ(db.Count("placestraveled"), 7); // minus Venice
  EXPECT_TRUE(db.DeleteObject(999));

  // Test removing relations
  CFileItemList temp2;
  vector<CStdString> fewerTables, fewerIndices;
  EXPECT_NO_THROW(db.DropManyToMany("placestraveled"));
  EXPECT_EQ(db.Count("placestraveled"), 0);
  EXPECT_FALSE(db.GetItemNav("placestraveled", temp2, ""));
  EXPECT_TRUE(db.GetAllTables(fewerTables));
  EXPECT_EQ(fewerTables.size(), 3);
  EXPECT_TRUE(db.GetAllIndices(fewerIndices));
  EXPECT_EQ(fewerIndices.size(), 4);

  vector<CStdString> fewererTables, fewererIndices;
  EXPECT_NO_THROW(db.DropIndex("littlegnomefriends"));
  EXPECT_TRUE(db.GetAllTables(fewererTables));
  EXPECT_EQ(fewererTables.size(), 3);
  EXPECT_TRUE(db.GetAllIndices(fewererIndices));
  EXPECT_EQ(fewererIndices.size(), 3);

  CFileItemList temp3;
  vector<CStdString> fewestTables, fewestIndices;
  EXPECT_NO_THROW(db.DropOneToMany("garden"));
  EXPECT_EQ(db.Count("garden"), 0);
  EXPECT_FALSE(db.GetItemNav("garden", temp3, ""));
  EXPECT_TRUE(db.GetAllTables(fewerTables));
  EXPECT_EQ(fewerTables.size(), 2);
  EXPECT_TRUE(db.GetAllIndices(fewerIndices));
  EXPECT_EQ(fewerIndices.size(), 1);

  EXPECT_THROW(db.DropManyToMany("placestraveled"), dbiplus::DbErrors);
  EXPECT_THROW(db.DropIndex("littlegnomefriends"), dbiplus::DbErrors);
  EXPECT_THROW(db.DropOneToMany("garden"), dbiplus::DbErrors);

  // Test adding relations
  CLawnGnome Gnome3;
  vector<CStdString> moreTables, moreIndices;
  EXPECT_NO_THROW(db.AddIndex("littlegnomefriends", "INTEGER"));
  EXPECT_TRUE(db.GetAllTables(moreTables));
  EXPECT_EQ(moreTables.size(), 2);
  EXPECT_TRUE(db.GetAllIndices(moreIndices));
  EXPECT_EQ(moreIndices.size(), 2);
  EXPECT_TRUE(db.GetObjectByIndex("littlegnomefriends", 1, &Gnome3));
  EXPECT_NE(Gnome3.m_name, "");

  vector<CStdString> evenMoreTables, evenMoreIndices;
  EXPECT_NO_THROW(db.AddOneToMany("garden", "VARCHAR(512)", true));
  EXPECT_EQ(db.Count("garden"), 3);
  EXPECT_TRUE(db.GetAllTables(evenMoreTables));
  EXPECT_EQ(evenMoreTables.size(), 3);
  EXPECT_TRUE(db.GetAllIndices(evenMoreIndices));
  EXPECT_EQ(evenMoreIndices.size(), 4);

  vector<CStdString> mostTables, mostIndices;
  EXPECT_NO_THROW(db.AddManyToMany("placestraveled", "TEXT", false));
  EXPECT_EQ(db.Count("placestraveled"), 7);
  EXPECT_TRUE(db.GetAllTables(mostTables));
  EXPECT_EQ(mostTables.size(), 5);
  EXPECT_TRUE(db.GetAllIndices(mostIndices));
  EXPECT_EQ(mostIndices.size(), 8);

  EXPECT_NO_THROW(db.AddIndex("littlegnomefriends", "INTEGER"));
  EXPECT_NO_THROW(db.AddOneToMany("garden", "VARCHAR(512)", true));
  EXPECT_NO_THROW(db.AddManyToMany("placestraveled", "TEXT", false));

  // Do a nav operation to verify that the data was restored
  // (also tests GetItemNav() over N:N items)
  CFileItemList temp4, temp5;
  EXPECT_TRUE(db.GetItemNav("garden", temp4, ""));
  EXPECT_EQ(temp4.Size(), 3);
  map<string, long> predicates2;
  predicates2["garden"] = 1;
  EXPECT_TRUE(db.GetItemNav("placestraveled", temp5, "", predicates2));
  EXPECT_EQ(temp5.Size(), 5);
}
