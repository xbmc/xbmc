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
  int            m_databaseId;

  CLawnGnome()
    : m_littleGnomeFriends(0), m_databaseId(-1)
  {
  }
  CLawnGnome(string name, string gardenName, int littleGnomeFriends)
    : m_name(name), m_gardenName(gardenName), m_littleGnomeFriends(littleGnomeFriends), m_databaseId(-1)
  {
  }

  virtual int GetID() const { return m_databaseId; }
  void TravelTo(const string &strPlace) { m_placesTraveled.push_back(strPlace); }

  virtual void Deserialize(const bson *document, int dbId)
  {
    bson_iterator it[1], subit[1];
    bson_type     type,  subtype;
    string        key,   subkey;

    bson_iterator_init(it, document);
    type = bson_iterator_type(it);
    while (type != BSON_EOO)
    {
      key = bson_iterator_key(it);
      if (type == BSON_STRING && key == "name") m_name = bson_iterator_string(it);
      else if (type == BSON_STRING && key == "garden") m_gardenName = bson_iterator_string(it);
      else if (type == BSON_INT && key == "littlegnomefriends") m_littleGnomeFriends = bson_iterator_int(it);
      else if (type == BSON_ARRAY && key == "placestraveled")
      {
        bson_iterator_subiterator(it, subit);
        subtype = bson_iterator_next(subit);
        while (subtype == BSON_STRING)
        {
          m_placesTraveled.push_back(bson_iterator_string(subit));
          subtype = bson_iterator_next(subit);
        }
      }
      type = bson_iterator_next(it);
    }
  }

  virtual void Serialize(bson *document) const
  {
    CStdString arrayKey;
    bson_init(document);
    bson_append_string(document, "name", m_name.c_str());
    bson_append_string(document, "garden", m_gardenName.c_str());
    bson_append_int(document, "littlegnomefriends", m_littleGnomeFriends);
    bson_append_start_array(document, "placestraveled");
    for (unsigned int i = 0; i < m_placesTraveled.size(); i++)
    {
      arrayKey.Format("%d", i);
      bson_append_string(document, arrayKey.c_str(), m_placesTraveled[i].c_str());
    }
    bson_append_finish_array(document);
    bson_finish(document);
  }
};


///////////////////////////////////////////////////////////////////////////////
// Test virtual database implementation: CGnomeDatabase
class CGnomeDatabase : public CDynamicDatabase
{
public:
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
  /* Gnome uniqueness is quantified by their name and garden. */
  virtual bool IsValid(const bson *object) const
  {
    return !GetField(object, "name").asString().empty() && !GetField(object, "garden").asString().empty();
  }
  virtual bool Exists(const bson *object, int &idObject)
  {
    if (!IsValid(object))
      return false;
    CStdString strSQL = PrepareSQL(
      "SELECT idGnome "
      "FROM gnome JOIN garden ON garden.idgarden=gnome.idgarden "
      "WHERE name='%s' AND garden='%s'",
      GetField(object, "name").asString().c_str(), GetField(object, "garden").asString().c_str()
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
  virtual CFileItem *CreateFileItem(const std::string &strBson, int id) const
  {
    CLawnGnome gnome;
    DeserializeBSON(strBson, id, &gnome);
    CFileItem *item = new CFileItem(gnome.m_name);
    item->SetPath(gnome.m_gardenName);
    return item;
  }
  // Returns true if the database is empty upon completion
  bool ClearGnomeDatabase()
  {
    try
    {
      auto_ptr<dbiplus::Dataset> pDS(m_pDB->CreateDataset());
      if (pDS->query("SELECT idGnome FROM Gnome"))
        while (!pDS->eof())
          { DeleteObject(pDS->fv(0).get_asInt()); pDS->next(); }
      pDS->close();
      return Count() == 0 && Count("garden") == 0 && Count("placestraveled") == 0;
    }
    catch (...)
    {
      return false;
    }
  }
  void DropGnomeDatabase()
  {
    assert(!m_sqlite);
    m_pDS->exec("DROP DATABASE IF EXISTS MyGnomes1");
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

  EXPECT_NO_THROW(db.DropGnomeDatabase());
}

void runTests(CGnomeDatabase &db)
{
  ASSERT_TRUE(db.IsOpen());
  
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
  EXPECT_EQ(indices.size(), 8);
  EXPECT_TRUE(has(indices, "idx_gnome_name"));
  EXPECT_TRUE(has(indices, "idx_gnome_garden"));
  EXPECT_TRUE(has(indices, "idx_gnome_littlegnomefriends"));
  EXPECT_TRUE(has(indices, "idx_garden_garden"));
  EXPECT_TRUE(has(indices, "idx_gnomelinkplacestraveled_gnome"));
  EXPECT_TRUE(has(indices, "idx_gnomelinkplacestraveled_placestraveled"));
  EXPECT_TRUE(has(indices, "idx_gnomelinkplacestraveled_link1"));
  EXPECT_TRUE(has(indices, "idx_gnomelinkplacestraveled_link2"));

  EXPECT_TRUE(db.ClearGnomeDatabase());

  // Set up our test data
  CLawnGnome Gnomeo        ("Gnomeo",         "Montague Garden",     4); // Venice, Zurich
  CLawnGnome Broomhilda    ("'\"\b\n\r\\n ",  "Ikea Garden",         4); // Zurich, Tokyo, Lisbon
  CLawnGnome Fruuuugenhagen("Fruuuugenhagen", "Ikea Garden",         1); // Belize, Tokyo, Lisbon
  CLawnGnome DrBrukunduke  ("Dr. Brukunduke", "Ikea Garden",         1); // Monte Carlo
  CLawnGnome Miserputin    ("Miserputin",     "Noodlepushin Garden", 4); // Lisbon, Georgia, Bangledash
  CLawnGnome Rekelpuke     ("Rekelpuke",      "RussiaIsCold Garden", 0);
  CLawnGnome Vuvelvive     ("Vuvelvive",      "",                    7); // Louisiana, Oregon, Vancouver, Wyoming
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

  // Try adding some data
  int id1, id2;
  EXPECT_EQ(db.AddObject(NULL), -1);
  EXPECT_NE(id1=db.AddObject(&Gnomeo), -1);
  EXPECT_EQ(db.AddObject(&Gnomeo, true), id1);
  EXPECT_EQ(db.AddObject(&Gnomeo, false), id1);
  EXPECT_NE(id2=db.AddObject(&Broomhilda), -1); // Dangerous name
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

  CFileItemList gardens, places, temp;
  EXPECT_TRUE(db.GetItemNav("garden", gardens, ""));
  EXPECT_EQ(gardens.Size(), 4); // Montague Garden, Ikea Garden, Noodlepushin Garden, RussiaIsCold Garden
  EXPECT_TRUE(db.GetItemNav("placestraveled", places, ""));
  EXPECT_EQ(places.Size(), 8); // Venice, Zurich, Tokyo, Lisbon, Belize, Monte Carlo, Georgia, Bangledash
  EXPECT_FALSE(db.GetItemNav("doesntexist", temp, ""));

  // Test N:N item
  int idLisbon = -1;
  EXPECT_TRUE(db.GetItemID("placestraveled", "Lisbon", idLisbon));
  EXPECT_NE(idLisbon, -1);
  // Do another test to verify that idLisbon is correct
  int idLisbonFromObject = -1;
  for (int i = 0; i < places.Size(); i++)
  {
    if (places[i]->GetLabel() == "Lisbon")
    {
      string place = places[i]->GetPath();
      if (!place.empty())
      {
        // Remove the slash
        place = place.substr(0, place.length() - 1);
        idLisbonFromObject = atol(place.c_str());
      }
      break;
    }
  }
  EXPECT_EQ(idLisbon, idLisbonFromObject);
  CVariant varLisbon;
  EXPECT_TRUE(db.GetItemByID("placestraveled", idLisbon, varLisbon));
  EXPECT_TRUE(varLisbon.asString() == "Lisbon");

  // Test ID of 1:N item
  int idIkea = -1;
  EXPECT_TRUE(db.GetItemID("garden", "Ikea Garden", idIkea));
  EXPECT_NE(idIkea, -1);
  int idIkeaFromObject = -1;
  for (int i = 0; i < gardens.Size(); i++)
  {
    if (gardens[i]->GetLabel() == "Ikea Garden")
    {
      string garden = gardens[i]->GetPath();
      if (!garden.empty())
      {
        // Remove the slash
        garden = garden.substr(0, garden.length() - 1);
        idIkeaFromObject = atol(garden.c_str());
      }
      break;
    }
  }
  EXPECT_EQ(idIkea, idIkeaFromObject);
  CVariant varIkea;
  EXPECT_TRUE(db.GetItemByID("garden", idIkea, varIkea));
  EXPECT_TRUE(varIkea.asString() == "Ikea Garden");

  int id;
  EXPECT_FALSE(db.GetItemID("doesntexist", "value", id));
  EXPECT_FALSE(db.GetItemByID("garden", 99, varLisbon));
  EXPECT_FALSE(db.GetItemByID("doesntexist", idLisbon, varLisbon));

  // Test item navigation with constraints
  CFileItemList gardensOnlyLisbon;
  map<string, long> conditionLisbon;
  conditionLisbon["placestraveled"] = idLisbon;
  EXPECT_TRUE(db.GetItemNav("garden", gardensOnlyLisbon, "", conditionLisbon));
  EXPECT_EQ(gardensOnlyLisbon.Size(), 2);
  // NOTE: This assumes that these gardens were added in a certain order
  CStdString garden1 = gardensOnlyLisbon[0]->GetLabel(),
             garden2 = gardensOnlyLisbon[1]->GetLabel();
  EXPECT_TRUE(garden1 == "Ikea Garden" || garden1 == "Noodlepushin Garden");
  EXPECT_TRUE(garden2 == "Ikea Garden" || garden2 == "Noodlepushin Garden");
  EXPECT_TRUE(garden1 != garden2);
  // Double constraints
  int idNoodlepushin = -1;
  EXPECT_TRUE(db.GetItemID("garden", "Noodlepushin Garden", idNoodlepushin));
  EXPECT_NE(idNoodlepushin, -1);
  map<string, long> conditionLisbonAndNoodlepushin = conditionLisbon;
  conditionLisbonAndNoodlepushin["garden"] = idNoodlepushin;
  gardensOnlyLisbon.Clear();
  EXPECT_TRUE(db.GetItemNav("garden", gardensOnlyLisbon, "", conditionLisbonAndNoodlepushin));
  EXPECT_EQ(gardensOnlyLisbon.Size(), 1);
  EXPECT_TRUE(gardensOnlyLisbon[0]->GetLabel() == "Noodlepushin Garden");
  
  // Test object nav
  CFileItemList objects;
  EXPECT_TRUE(db.GetObjectsNav(objects));
  EXPECT_EQ(objects.Size(), 6);
  objects.Clear();
  EXPECT_TRUE(db.GetObjectsNav(objects, conditionLisbon));
  EXPECT_EQ(objects.Size(), 3);
  objects.Clear();
  map<string, long> conditionLisbonAndIkea = conditionLisbon;
  conditionLisbonAndIkea["garden"] = idIkea;
  EXPECT_TRUE(db.GetObjectsNav(objects, conditionLisbonAndIkea));
  EXPECT_EQ(objects.Size(), 2);

  // Test deleting objects and check for orphans
  EXPECT_TRUE(db.DeleteObject(id1, false)); // Delete Gnomeo, leave orphans
  EXPECT_EQ(db.Count("garden"), 4);
  EXPECT_EQ(db.Count("placestraveled"), 8);
  EXPECT_TRUE(db.DeleteObject(id1));
  EXPECT_NE(id1=db.AddObject(&Gnomeo), -1); // Re-add, get the different ID
  EXPECT_EQ(db.Count("garden"), 4);
  EXPECT_EQ(db.Count("placestraveled"), 8);
  EXPECT_TRUE(db.DeleteObject(id1, true));  // Delete Gnomeo, delete orphans
  EXPECT_EQ(db.Count("garden"), 3); // minus Montague Garden
  EXPECT_EQ(db.Count("placestraveled"), 7); // minus Venice
  EXPECT_NE(db.AddObject(&Gnomeo), -1); // Put Gnomeo back
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
  EXPECT_EQ(db.Count("garden"), 4); // Montague Garden, Ikea Garden, Noodlepushin Garden, RussiaIsCold Garden
  EXPECT_TRUE(db.GetAllTables(evenMoreTables));
  EXPECT_EQ(evenMoreTables.size(), 3);
  EXPECT_TRUE(db.GetAllIndices(evenMoreIndices));
  EXPECT_EQ(evenMoreIndices.size(), 4);

  vector<CStdString> mostTables, mostIndices;
  EXPECT_NO_THROW(db.AddManyToMany("placestraveled", "TEXT", false));
  EXPECT_EQ(db.Count("placestraveled"), 8); // Venice, Zurich, Tokyo, Lisbon, Belize, Monte Carlo, Georgia, Bangledash
  EXPECT_TRUE(db.GetAllTables(mostTables));
  EXPECT_EQ(mostTables.size(), 5);
  EXPECT_TRUE(db.GetAllIndices(mostIndices));
  EXPECT_EQ(mostIndices.size(), 8);

  EXPECT_NO_THROW(db.AddIndex("littlegnomefriends", "INTEGER"));
  EXPECT_NO_THROW(db.AddOneToMany("garden", "VARCHAR(512)", true));
  EXPECT_NO_THROW(db.AddManyToMany("placestraveled", "TEXT", false));

  // Do a nav operation to verify that the correct data was restored
  EXPECT_TRUE(db.GetItemNav("garden", gardensOnlyLisbon, "", conditionLisbonAndNoodlepushin));
  EXPECT_EQ(gardensOnlyLisbon.Size(), 1);
  EXPECT_TRUE(gardensOnlyLisbon[0]->GetLabel() == "Noodlepushin Garden");

  // Test mass deletion
  EXPECT_TRUE(db.ClearGnomeDatabase());
}
