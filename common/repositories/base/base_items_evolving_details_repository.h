/**
 * DO NOT MODIFY THIS FILE
 *
 * This repository was automatically generated and is NOT to be modified directly.
 * Any repository modifications are meant to be made to the repository extending the base.
 * Any modifications to base repositories are to be made by the generator only
 *
 * @generator ./utils/scripts/generators/repository-generator.pl
 * @docs https://docs.eqemu.io/developer/repositories
 */

#ifndef EQEMU_BASE_ITEMS_EVOLVING_DETAILS_REPOSITORY_H
#define EQEMU_BASE_ITEMS_EVOLVING_DETAILS_REPOSITORY_H

#include "../../database.h"
#include "../../strings.h"
#include <ctime>

class BaseItemEvolvingDataRepository {
public:
	struct ItemsEvolvingDetails {
		uint32_t item_id;
		std::string item_name;
		int64_t  exp;
	};

	static std::string PrimaryKey()
	{
		return std::string("id");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"item_id",
			"item_name",
			"exp",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"item_id",
			"item_name",
			"exp",
		};
	}

	static std::string ColumnsRaw()
	{
		return std::string(Strings::Implode(", ", Columns()));
	}

	static std::string SelectColumnsRaw()
	{
		return std::string(Strings::Implode(", ", SelectColumns()));
	}

	static std::string TableName()
	{
		return std::string("item_evolving_data");
	}

	static std::string BaseSelect()
	{
		return fmt::format(
			"SELECT {} FROM {}",
			SelectColumnsRaw(),
			TableName()
		);
	}

	static std::string BaseInsert()
	{
		return fmt::format(
			"INSERT INTO {} ({}) ",
			TableName(),
			ColumnsRaw()
		);
	}

	static ItemsEvolvingDetails NewEntity()
	{
		ItemsEvolvingDetails e{};

		e.item_id           = 0;
		e.item_name         = "_";
		e.exp			    = 0;

		return e;
	}

	static ItemsEvolvingDetails GetItemsEvolvingDetails(
		const std::vector<ItemsEvolvingDetails> &items_evolving_detailss,
		int items_evolving_details_id
	)
	{
		for (auto &items_evolving_details : items_evolving_detailss) {
			if (items_evolving_details.item_id == items_evolving_details_id) {
				return items_evolving_details;
			}
		}

		return NewEntity();
	}

	static ItemsEvolvingDetails FindOne(
		Database& db,
		int items_evolving_details_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {} = {} LIMIT 1",
				BaseSelect(),
				PrimaryKey(),
				items_evolving_details_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			ItemsEvolvingDetails e{};

			e.item_id           = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.item_name			= row[1] ? static_cast<std::string>(row[1]) : "_";
			e.exp				= row[2] ? strtoll(row[2], nullptr, 10) : 0;

			return e;
		}

		return NewEntity();
	}

	static int DeleteOne(
		Database& db,
		int items_evolving_details_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				items_evolving_details_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		Database& db,
		const ItemsEvolvingDetails &e
	)
	{
		std::vector<std::string> v;

		auto columns = Columns();

		v.push_back(columns[1] + " = " + e.item_name);
		v.push_back(columns[2] + " = " + std::to_string(e.exp));

		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET {} WHERE {} = {}",
				TableName(),
				Strings::Implode(", ", v),
				PrimaryKey(),
				e.item_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static ItemsEvolvingDetails InsertOne(
		Database& db,
		ItemsEvolvingDetails e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.item_id));
		v.push_back(e.item_name);
		v.push_back(std::to_string(e.exp));

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseInsert(),
				Strings::Implode(",", v)
			)
		);

		if (results.Success()) {
			e.item_id = results.LastInsertedID();
			return e;
		}

		e = NewEntity();

		return e;
	}

	static int InsertMany(
		Database& db,
		const std::vector<ItemsEvolvingDetails> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.item_id));
			v.push_back(e.item_name);
			v.push_back(std::to_string(e.exp));

			insert_chunks.push_back("(" + Strings::Implode(",", v) + ")");
		}

		std::vector<std::string> v;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES {}",
				BaseInsert(),
				Strings::Implode(",", insert_chunks)
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static std::vector<ItemsEvolvingDetails> All(Database& db)
	{
		std::vector<ItemsEvolvingDetails> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			ItemsEvolvingDetails e{};

			e.item_id           = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.item_name			= row[1] ? static_cast<std::string>(row[1]) : "_";
			e.exp				= row[2] ? strtoll(row[2], nullptr, 10) : 0;

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::vector<ItemsEvolvingDetails> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<ItemsEvolvingDetails> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			ItemsEvolvingDetails e{};

			e.item_id = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.item_name = row[1] ? static_cast<std::string>(row[1]) : "_";
			e.exp = row[2] ? strtoll(row[2], nullptr, 10) : 0;

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static int DeleteWhere(Database& db, const std::string &where_filter)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {}",
				TableName(),
				where_filter
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int Truncate(Database& db)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"TRUNCATE TABLE {}",
				TableName()
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int64 GetMaxId(Database& db)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT COALESCE(MAX({}), 0) FROM {}",
				PrimaryKey(),
				TableName()
			)
		);

		return (results.Success() && results.begin()[0] ? strtoll(results.begin()[0], nullptr, 10) : 0);
	}

	static int64 Count(Database& db, const std::string &where_filter = "")
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT COUNT(*) FROM {} {}",
				TableName(),
				(where_filter.empty() ? "" : "WHERE " + where_filter)
			)
		);

		return (results.Success() && results.begin()[0] ? strtoll(results.begin()[0], nullptr, 10) : 0);
	}

	static std::string BaseReplace()
	{
		return fmt::format(
			"REPLACE INTO {} ({}) ",
			TableName(),
			ColumnsRaw()
		);
	}

	static int ReplaceOne(
		Database& db,
		const ItemsEvolvingDetails &e
	)
	{
		std::vector<std::string> v;


		v.push_back(std::to_string(e.item_id));
		v.push_back(e.item_name);
		v.push_back(std::to_string(e.exp));

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseReplace(),
				Strings::Implode(",", v)
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int ReplaceMany(
		Database& db,
		const std::vector<ItemsEvolvingDetails> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;


			v.push_back(std::to_string(e.item_id));
			v.push_back(e.item_name);
			v.push_back(std::to_string(e.exp));

			insert_chunks.push_back("(" + Strings::Implode(",", v) + ")");
		}

		std::vector<std::string> v;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES {}",
				BaseReplace(),
				Strings::Implode(",", insert_chunks)
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}
};

#endif //EQEMU_BASE_ITEMS_EVOLVING_DETAILS_REPOSITORY_H
