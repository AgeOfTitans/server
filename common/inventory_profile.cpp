/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2016 EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "inventory_profile.h"
#include "textures.h"
#include "eqemu_logsys.h"
//#include "classes.h"
//#include "global_define.h"
//#include "item_instance.h"
//#include "races.h"
//#include "rulesys.h"
//#include "shareddb.h"
#include "strings.h"

#include "../common/light_source.h"

//#include <limits.h>

#include <iostream>

std::list<EQ::ItemInstance*> dirty_inst;


//
// class ItemInstQueue
//
ItemInstQueue::~ItemInstQueue()
{
	for (auto iter = m_list.begin(); iter != m_list.end(); ++iter) {
		safe_delete(*iter);
	}
	m_list.clear();
}

// Put item onto back of queue
void ItemInstQueue::push(EQ::ItemInstance* inst)
{
	m_list.push_back(inst);
}

// Put item onto front of queue
void ItemInstQueue::push_front(EQ::ItemInstance* inst)
{
	m_list.push_front(inst);
}

// Remove item from front of queue
EQ::ItemInstance* ItemInstQueue::pop()
{
	if (m_list.empty())
		return nullptr;

	EQ::ItemInstance* inst = m_list.front();
	m_list.pop_front();
	return inst;
}

// Remove item from back of queue
EQ::ItemInstance* ItemInstQueue::pop_back()
{
	if (m_list.empty())
		return nullptr;

	EQ::ItemInstance* inst = m_list.back();
	m_list.pop_back();
	return inst;
}

// Look at item at front of queue
EQ::ItemInstance* ItemInstQueue::peek_front() const
{
	return (m_list.empty()) ? nullptr : m_list.front();
}


//
// class EQ::InventoryProfile
//
EQ::InventoryProfile::~InventoryProfile()
{
	for (auto iter = m_worn.begin(); iter != m_worn.end(); ++iter) {
		safe_delete(iter->second);
	}
	m_worn.clear();

	for (auto iter = m_inv.begin(); iter != m_inv.end(); ++iter) {
		safe_delete(iter->second);
	}
	m_inv.clear();

	for (auto iter = m_bank.begin(); iter != m_bank.end(); ++iter) {
		safe_delete(iter->second);
	}
	m_bank.clear();

	for (auto iter = m_shbank.begin(); iter != m_shbank.end(); ++iter) {
		safe_delete(iter->second);
	}
	m_shbank.clear();

	for (auto iter = m_trade.begin(); iter != m_trade.end(); ++iter) {
		safe_delete(iter->second);
	}
	m_trade.clear();
}

void EQ::InventoryProfile::SetInventoryVersion(versions::MobVersion inventory_version) {
	m_mob_version = versions::ValidateMobVersion(inventory_version);
	SetGMInventory(m_gm_inventory);
}

void EQ::InventoryProfile::SetGMInventory(bool gmi_flag) {
	m_gm_inventory = gmi_flag;

	m_lookup = inventory::DynamicLookup(m_mob_version, gmi_flag);
}

void EQ::InventoryProfile::CleanDirty() {
	auto iter = dirty_inst.begin();
	while (iter != dirty_inst.end()) {
		delete (*iter);
		++iter;
	}
	dirty_inst.clear();
}

void EQ::InventoryProfile::MarkDirty(ItemInstance *inst) {
	if (inst) {
		dirty_inst.push_back(inst);
	}
}

// Retrieve item at specified slot; returns false if item not found
EQ::ItemInstance* EQ::InventoryProfile::GetItem(int16 slot_id) const
{
	ItemInstance* result = nullptr;

	// Cursor
	if (slot_id == invslot::slotCursor) {
		// Cursor slot
		result = m_cursor.peek_front();
	}

	// Non bag slots
	else if (slot_id >= invslot::TRADE_BEGIN && slot_id <= invslot::TRADE_END) {
		result = _GetItem(m_trade, slot_id);
	}
	else if (slot_id >= invslot::SHARED_BANK_BEGIN && slot_id <= invslot::SHARED_BANK_END) {
		// Shared Bank slots
		result = _GetItem(m_shbank, slot_id);
	}
	else if (slot_id >= invslot::BANK_BEGIN && slot_id <= invslot::BANK_END) {
		// Bank slots
		result = _GetItem(m_bank, slot_id);
	}
	else if ((slot_id >= invslot::GENERAL_BEGIN && slot_id <= invslot::GENERAL_END)) {
		// Personal inventory slots
		result = _GetItem(m_inv, slot_id);
	}
	else if ((slot_id >= invslot::EQUIPMENT_BEGIN && slot_id <= invslot::EQUIPMENT_END) ||
		(slot_id >= invslot::TRIBUTE_BEGIN && slot_id <= invslot::TRIBUTE_END) ||
		(slot_id >= invslot::GUILD_TRIBUTE_BEGIN && slot_id <= invslot::GUILD_TRIBUTE_END)) {
		// Equippable slots (on body)
		result = _GetItem(m_worn, slot_id);
	}

	// Inner bag slots
	else if (slot_id >= invbag::TRADE_BAGS_BEGIN && slot_id <= invbag::TRADE_BAGS_END) {
		// Trade bag slots
		ItemInstance* inst = _GetItem(m_trade, InventoryProfile::CalcSlotId(slot_id));
		if (inst && inst->IsClassBag()) {
			result = inst->GetItem(InventoryProfile::CalcBagIdx(slot_id));
		}
	}
	else if (slot_id >= invbag::SHARED_BANK_BAGS_BEGIN && slot_id <= invbag::SHARED_BANK_BAGS_END) {
		// Shared Bank bag slots
		ItemInstance* inst = _GetItem(m_shbank, InventoryProfile::CalcSlotId(slot_id));
		if (inst && inst->IsClassBag()) {
			result = inst->GetItem(InventoryProfile::CalcBagIdx(slot_id));
		}
	}
	else if (slot_id >= invbag::BANK_BAGS_BEGIN && slot_id <= invbag::BANK_BAGS_END) {
		// Bank bag slots
		ItemInstance* inst = _GetItem(m_bank, InventoryProfile::CalcSlotId(slot_id));
		if (inst && inst->IsClassBag()) {
			result = inst->GetItem(InventoryProfile::CalcBagIdx(slot_id));
		}
	}
	else if (slot_id >= invbag::CURSOR_BAG_BEGIN && slot_id <= invbag::CURSOR_BAG_END) {
		// Cursor bag slots
		ItemInstance* inst = m_cursor.peek_front();
		if (inst && inst->IsClassBag()) {
			result = inst->GetItem(InventoryProfile::CalcBagIdx(slot_id));
		}
	}
	else if (slot_id >= invbag::GENERAL_BAGS_BEGIN && slot_id <= invbag::GENERAL_BAGS_END) {
		// Personal inventory bag slots
		ItemInstance* inst = _GetItem(m_inv, InventoryProfile::CalcSlotId(slot_id));
		if (inst && inst->IsClassBag()) {
			result = inst->GetItem(InventoryProfile::CalcBagIdx(slot_id));
		}
	}

	return result;
}

// Retrieve item at specified position within bag
EQ::ItemInstance* EQ::InventoryProfile::GetItem(int16 slot_id, uint8 bagidx) const
{
	return GetItem(InventoryProfile::CalcSlotId(slot_id, bagidx));
}

// Put an item into specified slot
int16 EQ::InventoryProfile::PutItem(int16 slot_id, const ItemInstance& inst)
{
	if (slot_id <= EQ::invslot::POSSESSIONS_END && slot_id >= EQ::invslot::POSSESSIONS_BEGIN) {
		if ((((uint64)1 << slot_id) & m_lookup->PossessionsBitmask) == 0)
			return EQ::invslot::SLOT_INVALID;
	}
	else if (slot_id <= EQ::invbag::GENERAL_BAGS_END && slot_id >= EQ::invbag::GENERAL_BAGS_BEGIN) {
		auto temp_slot = EQ::invslot::GENERAL_BEGIN + ((slot_id - EQ::invbag::GENERAL_BAGS_BEGIN) / EQ::invbag::SLOT_COUNT);
		if ((((uint64)1 << temp_slot) & m_lookup->PossessionsBitmask) == 0)
			return EQ::invslot::SLOT_INVALID;
	}
	else if (slot_id <= EQ::invslot::BANK_END && slot_id >= EQ::invslot::BANK_BEGIN) {
		if ((slot_id - EQ::invslot::BANK_BEGIN) >= m_lookup->InventoryTypeSize.Bank)
			return EQ::invslot::SLOT_INVALID;
	}
	else if (slot_id <= EQ::invbag::BANK_BAGS_END && slot_id >= EQ::invbag::BANK_BAGS_BEGIN) {
		auto temp_slot = (slot_id - EQ::invbag::BANK_BAGS_BEGIN) / EQ::invbag::SLOT_COUNT;
		if (temp_slot >= m_lookup->InventoryTypeSize.Bank)
			return EQ::invslot::SLOT_INVALID;
	}

	// Clean up item already in slot (if exists)
	DeleteItem(slot_id);

	if (!inst) {
		// User is effectively deleting the item
		// in the slot, why hold a null ptr in map<>?
		return slot_id;
	}

	// Delegate to internal method
	return _PutItem(slot_id, inst.Clone());
}

int16 EQ::InventoryProfile::PushCursor(const ItemInstance &inst) {
	m_cursor.push(inst.Clone());
	return invslot::slotCursor;
}

EQ::ItemInstance* EQ::InventoryProfile::GetCursorItem() {
	return m_cursor.peek_front();
}

// Swap items in inventory
bool EQ::InventoryProfile::SwapItem(
	int16 source_slot,
	int16 destination_slot,
	SwapItemFailState &fail_state,
	uint16 race_id,
	uint8 class_id,
	uint32 deity_id,
	uint8 level
) {
	fail_state = swapInvalid;

	if (source_slot <= EQ::invslot::POSSESSIONS_END && source_slot >= EQ::invslot::POSSESSIONS_BEGIN) {
		if ((((uint64) 1 << source_slot) & m_lookup->PossessionsBitmask) == 0) {
			fail_state = swapNotAllowed;
			return false;
		}
	}
	else if (source_slot <= EQ::invbag::GENERAL_BAGS_END && source_slot >= EQ::invbag::GENERAL_BAGS_BEGIN) {
		auto temp_slot = EQ::invslot::GENERAL_BEGIN + ((source_slot - EQ::invbag::GENERAL_BAGS_BEGIN) / EQ::invbag::SLOT_COUNT);
		if ((((uint64)1 << temp_slot) & m_lookup->PossessionsBitmask) == 0) {
			fail_state = swapNotAllowed;
			return false;
		}
	}
	else if (source_slot <= EQ::invslot::BANK_END && source_slot >= EQ::invslot::BANK_BEGIN) {
		if ((source_slot - EQ::invslot::BANK_BEGIN) >= m_lookup->InventoryTypeSize.Bank) {
			fail_state = swapNotAllowed;
			return false;
		}
	}
	else if (source_slot <= EQ::invbag::BANK_BAGS_END && source_slot >= EQ::invbag::BANK_BAGS_BEGIN) {
		auto temp_slot = (source_slot - EQ::invbag::BANK_BAGS_BEGIN) / EQ::invbag::SLOT_COUNT;
		if (temp_slot >= m_lookup->InventoryTypeSize.Bank) {
			fail_state = swapNotAllowed;
			return false;
		}
	}

	if (destination_slot <= EQ::invslot::POSSESSIONS_END && destination_slot >= EQ::invslot::POSSESSIONS_BEGIN) {
		if ((((uint64)1 << destination_slot) & m_lookup->PossessionsBitmask) == 0) {
			fail_state = swapNotAllowed;
			return false;
		}
	}
	else if (destination_slot <= EQ::invbag::GENERAL_BAGS_END && destination_slot >= EQ::invbag::GENERAL_BAGS_BEGIN) {
		auto temp_slot = EQ::invslot::GENERAL_BEGIN + ((destination_slot - EQ::invbag::GENERAL_BAGS_BEGIN) / EQ::invbag::SLOT_COUNT);
		if ((((uint64)1 << temp_slot) & m_lookup->PossessionsBitmask) == 0) {
			fail_state = swapNotAllowed;
			return false;
		}
	}
	else if (destination_slot <= EQ::invslot::BANK_END && destination_slot >= EQ::invslot::BANK_BEGIN) {
		if ((destination_slot - EQ::invslot::BANK_BEGIN) >= m_lookup->InventoryTypeSize.Bank) {
			fail_state = swapNotAllowed;
			return false;
		}
	}
	else if (destination_slot <= EQ::invbag::BANK_BAGS_END && destination_slot >= EQ::invbag::BANK_BAGS_BEGIN) {
		auto temp_slot = (destination_slot - EQ::invbag::BANK_BAGS_BEGIN) / EQ::invbag::SLOT_COUNT;
		if (temp_slot >= m_lookup->InventoryTypeSize.Bank) {
			fail_state = swapNotAllowed;
			return false;
		}
	}

	// Temp holding areas for source and destination
	ItemInstance *source_item_instance      = GetItem(source_slot);
	ItemInstance *destination_item_instance = GetItem(destination_slot);

	if (source_item_instance) {
		if (!source_item_instance->IsSlotAllowed(destination_slot)) {
			fail_state = swapNotAllowed;
			return false;
		}

		source_item_instance->SetEvolveEquipped(false);
		if ((destination_slot >= invslot::EQUIPMENT_BEGIN && destination_slot <= invslot::EQUIPMENT_END)) {
			auto source_item = source_item_instance->GetItem();
			if (!source_item) {
				fail_state = swapNullData;
				return false;
			}
			if (race_id && class_id && !source_item->IsEquipable(race_id, class_id)) {
				fail_state = swapRaceClass;
				return false;
			}
			if (deity_id && source_item->Deity && !(Deity::GetBitmask(deity_id) & source_item->Deity)) {
				fail_state = swapDeity;
				return false;
			}
			if (level && source_item->ReqLevel && level < source_item->ReqLevel) {
				fail_state = swapLevel;
				return false;
			}
			if (source_item_instance->IsEvolving() > 0) {
				source_item_instance->SetEvolveEquipped(true);
			}
		}
	}

	if (destination_item_instance) {
		if (!destination_item_instance->IsSlotAllowed(source_slot)) {
			fail_state = swapNotAllowed;
			return false;
		}

		destination_item_instance->SetEvolveEquipped(false);
		if ((source_slot >= invslot::EQUIPMENT_BEGIN && source_slot <= invslot::EQUIPMENT_END)) {
			auto destination_item = destination_item_instance->GetItem();
			if (!destination_item) {
				fail_state = swapNullData;
				return false;
			}
			if (race_id && class_id && !destination_item->IsEquipable(race_id, class_id)) {
				fail_state = swapRaceClass;
				return false;
			}
			if (deity_id && destination_item->Deity && !(Deity::GetBitmask(deity_id) & destination_item->Deity)) {
				fail_state = swapDeity;
				return false;
			}
			if (level && destination_item->ReqLevel && level < destination_item->ReqLevel) {
				fail_state = swapLevel;
				return false;
			}
			if (destination_item_instance->IsEvolving()) {
				destination_item_instance->SetEvolveEquipped(true);
			}
		}
	}

	_PutItem(source_slot, destination_item_instance); // Assign destination -> source
	_PutItem(destination_slot, source_item_instance); // Assign source -> destination

	fail_state = swapPass;

	return true;
}

// Remove item from inventory (with memory delete)
bool EQ::InventoryProfile::DeleteItem(int16 slot_id, int16 quantity) {
	// Pop item out of inventory map (or queue)
	ItemInstance *item_to_delete = PopItem(slot_id);

	// Determine if object should be fully deleted, or
	// just a quantity of charges of the item can be deleted
	if (item_to_delete && (quantity > 0)) {

		item_to_delete->SetCharges(item_to_delete->GetCharges() - quantity);

		// If there are no charges left on the item,
		if (item_to_delete->GetCharges() <= 0) {
			// If the item is stackable (e.g arrows), or
			// the item is not a charged item, or is expendable, delete it
			if (
				item_to_delete->IsStackable() ||
				item_to_delete->GetItem()->MaxCharges == 0 ||
				item_to_delete->IsExpendable()
			) {
				// Item can now be destroyed
				InventoryProfile::MarkDirty(item_to_delete);
				return true;
			}
		}

		// Charges still exist, or it is a charged item that is not expendable. Put back into inventory
		_PutItem(slot_id, item_to_delete);
		return false;
	}

	InventoryProfile::MarkDirty(item_to_delete);
	return true;

}

// Checks All items in a bag for No Drop
bool EQ::InventoryProfile::CheckNoDrop(int16 slot_id, bool recurse)
{
	ItemInstance* inst = GetItem(slot_id);
	if (!inst)
		return false;

	return (!inst->IsDroppable(recurse));
}

// Remove item from bucket without memory delete
// Returns item pointer if full delete was successful
EQ::ItemInstance* EQ::InventoryProfile::PopItem(int16 slot_id)
{
	ItemInstance* p = nullptr;

	if (slot_id == invslot::slotCursor) {
		p = m_cursor.pop();
	}
	else if (slot_id >= invslot::EQUIPMENT_BEGIN && slot_id <= invslot::EQUIPMENT_END) {
		p = m_worn[slot_id];
		m_worn.erase(slot_id);
	}
	else if (slot_id >= invslot::GENERAL_BEGIN && slot_id <= invslot::GENERAL_END) {
		p = m_inv[slot_id];
		m_inv.erase(slot_id);
	}
	else if (slot_id >= invslot::TRIBUTE_BEGIN && slot_id <= invslot::TRIBUTE_END) {
		p = m_worn[slot_id];
		m_worn.erase(slot_id);
	}
	else if (slot_id >= invslot::GUILD_TRIBUTE_BEGIN && slot_id <= invslot::GUILD_TRIBUTE_END) {
		p = m_worn[slot_id];
		m_worn.erase(slot_id);
	}
	else if (slot_id >= invslot::BANK_BEGIN && slot_id <= invslot::BANK_END) {
		p = m_bank[slot_id];
		m_bank.erase(slot_id);
	}
	else if (slot_id >= invslot::SHARED_BANK_BEGIN && slot_id <= invslot::SHARED_BANK_END) {
		p = m_shbank[slot_id];
		m_shbank.erase(slot_id);
	}
	else if (slot_id >= invslot::TRADE_BEGIN && slot_id <= invslot::TRADE_END) {
		p = m_trade[slot_id];
		m_trade.erase(slot_id);
	}
	else {
		// Is slot inside bag?
		ItemInstance* baginst = GetItem(InventoryProfile::CalcSlotId(slot_id));
		if (baginst != nullptr && baginst->IsClassBag()) {
			p = baginst->PopItem(InventoryProfile::CalcBagIdx(slot_id));
		}
	}

	// Return pointer that needs to be deleted (or otherwise managed)
	return p;
}

bool EQ::InventoryProfile::HasSpaceForItem(const ItemData *ItemToTry, int16 Quantity) {

	if (ItemToTry->Stackable) {

		for (int16 i = invslot::GENERAL_BEGIN; i <= invslot::GENERAL_END; i++) {
			if ((((uint64)1 << i) & m_lookup->PossessionsBitmask) == 0)
				continue;

			ItemInstance* InvItem = GetItem(i);

			if (InvItem && (InvItem->GetItem()->ID == ItemToTry->ID) && (InvItem->GetCharges() < InvItem->GetItem()->StackSize)) {

				int ChargeSlotsLeft = InvItem->GetItem()->StackSize - InvItem->GetCharges();

				if (Quantity <= ChargeSlotsLeft)
					return true;

				Quantity -= ChargeSlotsLeft;

			}
			if (InvItem && InvItem->IsClassBag()) {

				int16 BaseSlotID = InventoryProfile::CalcSlotId(i, invbag::SLOT_BEGIN);
				uint8 BagSize = InvItem->GetItem()->BagSlots;
				for (uint8 BagSlot = invbag::SLOT_BEGIN; BagSlot < BagSize; BagSlot++) {

					InvItem = GetItem(BaseSlotID + BagSlot);

					if (InvItem && (InvItem->GetItem()->ID == ItemToTry->ID) &&
						(InvItem->GetCharges() < InvItem->GetItem()->StackSize)) {

						int ChargeSlotsLeft = InvItem->GetItem()->StackSize - InvItem->GetCharges();

						if (Quantity <= ChargeSlotsLeft)
							return true;

						Quantity -= ChargeSlotsLeft;
					}
				}
			}
		}
	}

	for (int16 i = invslot::GENERAL_BEGIN; i <= invslot::GENERAL_END; i++) {
		if ((((uint64)1 << i) & m_lookup->PossessionsBitmask) == 0)
			continue;

		ItemInstance* InvItem = GetItem(i);

		if (!InvItem) {

			if (!ItemToTry->Stackable) {

				if (Quantity == 1)
					return true;
				else
					Quantity--;
			}
			else {
				if (Quantity <= ItemToTry->StackSize)
					return true;
				else
					Quantity -= ItemToTry->StackSize;
			}

		}
		else if (InvItem->IsClassBag() && CanItemFitInContainer(ItemToTry, InvItem->GetItem())) {

			int16 BaseSlotID = InventoryProfile::CalcSlotId(i, invbag::SLOT_BEGIN);

			uint8 BagSize = InvItem->GetItem()->BagSlots;

			for (uint8 BagSlot = invbag::SLOT_BEGIN; BagSlot < BagSize; BagSlot++) {

				InvItem = GetItem(BaseSlotID + BagSlot);

				if (!InvItem) {
					if (!ItemToTry->Stackable) {

						if (Quantity == 1)
							return true;
						else
							Quantity--;
					}
					else {
						if (Quantity <= ItemToTry->StackSize)
							return true;
						else
							Quantity -= ItemToTry->StackSize;
					}
				}
			}
		}
	}

	return false;

}

// Checks that user has at least 'quantity' number of items in a given inventory slot
// Returns first slot it was found in, or SLOT_INVALID if not found

bool EQ::InventoryProfile::HasAugmentEquippedByID(uint32 item_id)
{
	bool has_equipped = false;
	ItemInstance* item = nullptr;

	for (int slot_id = EQ::invslot::EQUIPMENT_BEGIN; slot_id <= EQ::invslot::EQUIPMENT_END; ++slot_id) {
		item = GetItem(slot_id);
		if (item && item->ContainsAugmentByID(item_id)) {
			has_equipped = true;
			break;
		}
	}

	return has_equipped;
}

int EQ::InventoryProfile::CountAugmentEquippedByID(uint32 item_id)
{
	int quantity = 0;
	ItemInstance* item = nullptr;

	for (int slot_id = EQ::invslot::EQUIPMENT_BEGIN; slot_id <= EQ::invslot::EQUIPMENT_END; ++slot_id) {
		item = GetItem(slot_id);
		if (item && item->ContainsAugmentByID(item_id)) {
			quantity += item->CountAugmentByID(item_id);
		}
	}

	return quantity;
}

bool EQ::InventoryProfile::HasItemEquippedByID(uint32 item_id)
{
	bool has_equipped = false;
	ItemInstance* item = nullptr;

	for (int slot_id = EQ::invslot::EQUIPMENT_BEGIN; slot_id <= EQ::invslot::EQUIPMENT_END; ++slot_id) {
		item = GetItem(slot_id);
		if (item && item->GetID() == item_id) {
			has_equipped = true;
			break;
		}
	}

	return has_equipped;
}

int EQ::InventoryProfile::CountItemEquippedByID(uint32 item_id)
{
	int quantity = 0;
	ItemInstance* item = nullptr;

	for (int slot_id = EQ::invslot::EQUIPMENT_BEGIN; slot_id <= EQ::invslot::EQUIPMENT_END; ++slot_id) {
		item = GetItem(slot_id);
		if (item && item->GetID() == item_id) {
			quantity += item->IsStackable() ? item->GetCharges() : 1;
		}
	}

	return quantity;
}

//This function has a flaw in that it only returns the last stack that it looked at
//when quantity is greater than 1 and not all of quantity can be found in 1 stack.
int16 EQ::InventoryProfile::HasItem(uint32 item_id, uint8 quantity, uint8 where)
{
	int16 slot_id = INVALID_INDEX;

	//Altered by Father Nitwit to support a specification of
	//where to search, with a default value to maintain compatibility

	// Check each inventory bucket
	if (where & invWhereWorn) {
		slot_id = _HasItem(m_worn, item_id, quantity);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	if (where & invWherePersonal) {
		slot_id = _HasItem(m_inv, item_id, quantity);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	if (where & invWhereBank) {
		slot_id = _HasItem(m_bank, item_id, quantity);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	if (where & invWhereSharedBank) {
		slot_id = _HasItem(m_shbank, item_id, quantity);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	if (where & invWhereTrading) {
		slot_id = _HasItem(m_trade, item_id, quantity);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	// Behavioral change - Limbo is no longer checked due to improper handling of return value
	if (where & invWhereCursor) {
		// Check cursor queue
		slot_id = _HasItem(m_cursor, item_id, quantity);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	return slot_id;
}

//this function has the same quantity flaw mentioned above in HasItem()
int16 EQ::InventoryProfile::HasItemByUse(uint8 use, uint8 quantity, uint8 where)
{
	int16 slot_id = INVALID_INDEX;

	// Check each inventory bucket
	if (where & invWhereWorn) {
		slot_id = _HasItemByUse(m_worn, use, quantity);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	if (where & invWherePersonal) {
		slot_id = _HasItemByUse(m_inv, use, quantity);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	if (where & invWhereBank) {
		slot_id = _HasItemByUse(m_bank, use, quantity);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	if (where & invWhereSharedBank) {
		slot_id = _HasItemByUse(m_shbank, use, quantity);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	if (where & invWhereTrading) {
		slot_id = _HasItemByUse(m_trade, use, quantity);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	// Behavioral change - Limbo is no longer checked due to improper handling of return value
	if (where & invWhereCursor) {
		// Check cursor queue
		slot_id = _HasItemByUse(m_cursor, use, quantity);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	return slot_id;
}

int16 EQ::InventoryProfile::HasItemByLoreGroup(uint32 loregroup, uint8 where)
{
	int16 slot_id = INVALID_INDEX;

	// Check each inventory bucket
	if (where & invWhereWorn) {
		slot_id = _HasItemByLoreGroup(m_worn, loregroup);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	if (where & invWherePersonal) {
		slot_id = _HasItemByLoreGroup(m_inv, loregroup);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	if (where & invWhereBank) {
		slot_id = _HasItemByLoreGroup(m_bank, loregroup);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	if (where & invWhereSharedBank) {
		slot_id = _HasItemByLoreGroup(m_shbank, loregroup);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	if (where & invWhereTrading) {
		slot_id = _HasItemByLoreGroup(m_trade, loregroup);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	// Behavioral change - Limbo is no longer checked due to improper handling of return value
	if (where & invWhereCursor) {
		// Check cursor queue
		slot_id = _HasItemByLoreGroup(m_cursor, loregroup);
		if (slot_id != INVALID_INDEX)
			return slot_id;
	}

	return slot_id;
}

// Locate an available inventory slot
// Returns slot_id when there's one available, else SLOT_INVALID
int16 EQ::InventoryProfile::FindFreeSlot(bool for_bag, bool try_cursor, uint8 min_size, bool is_arrow)
{
	const int16 last_bag_slot = (RuleI(World, ExpansionSettings) == -1 || RuleI(World, ExpansionSettings) & EQ::expansions::bitHoT) ? EQ::invslot::slotGeneral10 : EQ::invslot::slotGeneral8;

	for (int16 i = invslot::GENERAL_BEGIN; i <= last_bag_slot; i++) { // Check basic inventory
		if ((((uint64) 1 << i) & m_lookup->PossessionsBitmask) == 0) {
			continue;
		}

		if (!GetItem(i)) {
			return i; // Found available slot in personal inventory
		}
	}

	if (!for_bag) {
		for (int16 i = invslot::GENERAL_BEGIN; i <= last_bag_slot; i++) {
			if ((((uint64) 1 << i) & m_lookup->PossessionsBitmask) == 0) {
				continue;
			}

			const auto *inst = GetItem(i);
			if (inst && inst->IsClassBag() && inst->GetItem()->BagSize >= min_size) {
				if (inst->GetItem()->BagType == item::BagTypeQuiver &&
					inst->GetItem()->ItemType != item::ItemTypeArrow) {
					continue;
				}

				const int16 base_slot_id = InventoryProfile::CalcSlotId(i, invbag::SLOT_BEGIN);

				const uint8 slots = inst->GetItem()->BagSlots;
				for (uint8 j = invbag::SLOT_BEGIN; j < slots; j++) {
					if (!GetItem(base_slot_id + j)) {
						// Found available slot within bag
						return (base_slot_id + j);
					}
				}
			}
		}
	}

	if (try_cursor) {
		// Always room on cursor (it's a queue)
		// (we may wish to cap this in the future)
		return invslot::slotCursor;
	}

	// No available slots
	return INVALID_INDEX;
}

// This is a mix of HasSpaceForItem and FindFreeSlot..due to existing coding behavior, it was better to add a new helper function...
int16 EQ::InventoryProfile::FindFreeSlotForTradeItem(const ItemInstance* inst, int16 general_start, uint8 bag_start) {
	// Do not arbitrarily use this function..it is designed for use with Client::ResetTrade() and Client::FinishTrade().
	// If you have a need, use it..but, understand it is not a suitable replacement for InventoryProfile::FindFreeSlot().
	//
	// I'll probably implement a bitmask in the new inventory system to avoid having to adjust stack bias

	if ((general_start < invslot::GENERAL_BEGIN) || (general_start > invslot::GENERAL_END))
		return INVALID_INDEX;
	if (bag_start > invbag::SLOT_END)
		return INVALID_INDEX;

	if (!inst || !inst->GetID())
		return INVALID_INDEX;

	// step 1: find room for bags (caller should really ask for slots for bags first to avoid sending them to cursor..and bag item loss)
	if (inst->IsClassBag()) {
		for (int16 free_slot = general_start; free_slot <= invslot::GENERAL_END; ++free_slot) {
			if ((((uint64)1 << free_slot) & m_lookup->PossessionsBitmask) == 0)
				continue;

			if (!m_inv[free_slot])
				return free_slot;
		}

		return invslot::slotCursor; // return cursor since bags do not stack and will not fit inside other bags..yet...)
	}

	// step 2: find partial room for stackables
	if (inst->IsStackable()) {
		for (int16 free_slot = general_start; free_slot <= invslot::GENERAL_END; ++free_slot) {
			if ((((uint64)1 << free_slot) & m_lookup->PossessionsBitmask) == 0)
				continue;

			const ItemInstance* main_inst = m_inv[free_slot];

			if (!main_inst)
				continue;

			if ((main_inst->GetID() == inst->GetID()) && (main_inst->GetCharges() < main_inst->GetItem()->StackSize))
				return free_slot;
		}

		for (int16 free_slot = general_start; free_slot <= invslot::GENERAL_END; ++free_slot) {
			if ((((uint64)1 << free_slot) & m_lookup->PossessionsBitmask) == 0)
				continue;

			const ItemInstance* main_inst = m_inv[free_slot];

			if (!main_inst)
				continue;

			if (main_inst->IsClassBag()) { // if item-specific containers already have bad items, we won't fix it here...
				uint8 _bag_start = (free_slot > general_start) ? invbag::SLOT_BEGIN : bag_start;
				for (uint8 free_bag_slot = _bag_start; (free_bag_slot < main_inst->GetItem()->BagSlots) && (free_bag_slot <= invbag::SLOT_END); ++free_bag_slot) {
					const ItemInstance* sub_inst = main_inst->GetItem(free_bag_slot);

					if (!sub_inst)
						continue;

					if ((sub_inst->GetID() == inst->GetID()) && (sub_inst->GetCharges() < sub_inst->GetItem()->StackSize))
						return InventoryProfile::CalcSlotId(free_slot, free_bag_slot);
				}
			}
		}
	}

	// step 3a: find room for container-specific items (ItemClassArrow)
	if (inst->GetItem()->ItemType == item::ItemTypeArrow) {
		for (int16 free_slot = general_start; free_slot <= invslot::GENERAL_END; ++free_slot) {
			if ((((uint64)1 << free_slot) & m_lookup->PossessionsBitmask) == 0)
				continue;

			const ItemInstance* main_inst = m_inv[free_slot];

			if (!main_inst || (main_inst->GetItem()->BagType != item::BagTypeQuiver) || !main_inst->IsClassBag())
				continue;

			uint8 _bag_start = (free_slot > general_start) ? invbag::SLOT_BEGIN : bag_start;
			for (uint8 free_bag_slot = _bag_start; (free_bag_slot < main_inst->GetItem()->BagSlots) && (free_bag_slot <= invbag::SLOT_END); ++free_bag_slot) {
				if (!main_inst->GetItem(free_bag_slot))
					return InventoryProfile::CalcSlotId(free_slot, free_bag_slot);
			}
		}
	}

	// step 3b: find room for container-specific items (ItemClassSmallThrowing)
	if (inst->GetItem()->ItemType == item::ItemTypeSmallThrowing) {
		for (int16 free_slot = general_start; free_slot <= invslot::GENERAL_END; ++free_slot) {
			if ((((uint64)1 << free_slot) & m_lookup->PossessionsBitmask) == 0)
				continue;

			const ItemInstance* main_inst = m_inv[free_slot];

			if (!main_inst || (main_inst->GetItem()->BagType != item::BagTypeBandolier) || !main_inst->IsClassBag())
				continue;

			uint8 _bag_start = (free_slot > general_start) ? invbag::SLOT_BEGIN : bag_start;
			for (uint8 free_bag_slot = _bag_start; (free_bag_slot < main_inst->GetItem()->BagSlots) && (free_bag_slot <= invbag::SLOT_END); ++free_bag_slot) {
				if (!main_inst->GetItem(free_bag_slot))
					return InventoryProfile::CalcSlotId(free_slot, free_bag_slot);
			}
		}
	}

	// step 4: just find an empty slot
	for (int16 free_slot = general_start; free_slot <= invslot::GENERAL_END; ++free_slot) {
		if ((((uint64)1 << free_slot) & m_lookup->PossessionsBitmask) == 0)
			continue;

		const ItemInstance* main_inst = m_inv[free_slot];

		if (!main_inst)
			return free_slot;
	}

	for (int16 free_slot = general_start; free_slot <= invslot::GENERAL_END; ++free_slot) {
		if ((((uint64)1 << free_slot) & m_lookup->PossessionsBitmask) == 0)
			continue;

		const ItemInstance* main_inst = m_inv[free_slot];

		if (main_inst && main_inst->IsClassBag()) {
			if ((main_inst->GetItem()->BagSize < inst->GetItem()->Size) || (main_inst->GetItem()->BagType == item::BagTypeBandolier) || (main_inst->GetItem()->BagType == item::BagTypeQuiver))
				continue;

			uint8 _bag_start = (free_slot > general_start) ? invbag::SLOT_BEGIN : bag_start;
			for (uint8 free_bag_slot = _bag_start; (free_bag_slot < main_inst->GetItem()->BagSlots) && (free_bag_slot <= invbag::SLOT_END); ++free_bag_slot) {
				if (!main_inst->GetItem(free_bag_slot))
					return InventoryProfile::CalcSlotId(free_slot, free_bag_slot);
			}
		}
	}

	//return INVALID_INDEX; // everything else pushes to the cursor
	return invslot::slotCursor;
}

// Opposite of below: Get parent bag slot_id from a slot inside of bag
int16 EQ::InventoryProfile::CalcSlotId(int16 slot_id) {
	int16 parent_slot_id = INVALID_INDEX;

	// this is not a bag range... using this risks over-writing existing items
	//else if (slot_id >= EmuConstants::BANK_BEGIN && slot_id <= EmuConstants::BANK_END)
	//	parent_slot_id = EmuConstants::BANK_BEGIN + (slot_id - EmuConstants::BANK_BEGIN) / EmuConstants::ITEM_CONTAINER_SIZE;
	//else if (slot_id >= 3100 && slot_id <= 3179) should be {3031..3110}..where did this range come from!!? (verified db save range)

	if (slot_id >= invbag::GENERAL_BAGS_BEGIN && slot_id <= invbag::GENERAL_BAGS_END) {
		parent_slot_id = invslot::GENERAL_BEGIN + (slot_id - invbag::GENERAL_BAGS_BEGIN) / invbag::SLOT_COUNT;
	}
	else if (slot_id >= invbag::CURSOR_BAG_BEGIN && slot_id <= invbag::CURSOR_BAG_END) {
		parent_slot_id = invslot::slotCursor;
	}
	else if (slot_id >= invbag::BANK_BAGS_BEGIN && slot_id <= invbag::BANK_BAGS_END) {
		parent_slot_id = invslot::BANK_BEGIN + (slot_id - invbag::BANK_BAGS_BEGIN) / invbag::SLOT_COUNT;
	}
	else if (slot_id >= invbag::SHARED_BANK_BAGS_BEGIN && slot_id <= invbag::SHARED_BANK_BAGS_END) {
		parent_slot_id = invslot::SHARED_BANK_BEGIN + (slot_id - invbag::SHARED_BANK_BAGS_BEGIN) / invbag::SLOT_COUNT;
	}
	else if (slot_id >= invbag::TRADE_BAGS_BEGIN && slot_id <= invbag::TRADE_BAGS_END) {
		parent_slot_id = invslot::TRADE_BEGIN + (slot_id - invbag::TRADE_BAGS_BEGIN) / invbag::SLOT_COUNT;
	}

	return parent_slot_id;
}

// Calculate slot_id for an item within a bag
int16 EQ::InventoryProfile::CalcSlotId(int16 bagslot_id, uint8 bagidx) {
	if (!InventoryProfile::SupportsContainers(bagslot_id))
		return INVALID_INDEX;

	int16 slot_id = INVALID_INDEX;

	if (bagslot_id == invslot::slotCursor || bagslot_id == 8000) {
		slot_id = invbag::CURSOR_BAG_BEGIN + bagidx;
	}
	else if (bagslot_id >= invslot::GENERAL_BEGIN && bagslot_id <= invslot::GENERAL_END) {
		slot_id = invbag::GENERAL_BAGS_BEGIN + (bagslot_id - invslot::GENERAL_BEGIN) * invbag::SLOT_COUNT + bagidx;
	}
	else if (bagslot_id >= invslot::BANK_BEGIN && bagslot_id <= invslot::BANK_END) {
		slot_id = invbag::BANK_BAGS_BEGIN + (bagslot_id - invslot::BANK_BEGIN) * invbag::SLOT_COUNT + bagidx;
	}
	else if (bagslot_id >= invslot::SHARED_BANK_BEGIN && bagslot_id <= invslot::SHARED_BANK_END) {
		slot_id = invbag::SHARED_BANK_BAGS_BEGIN + (bagslot_id - invslot::SHARED_BANK_BEGIN) * invbag::SLOT_COUNT + bagidx;
	}
	else if (bagslot_id >= invslot::TRADE_BEGIN && bagslot_id <= invslot::TRADE_END) {
		slot_id = invbag::TRADE_BAGS_BEGIN + (bagslot_id - invslot::TRADE_BEGIN) * invbag::SLOT_COUNT + bagidx;
	}

	return slot_id;
}

uint8 EQ::InventoryProfile::CalcBagIdx(int16 slot_id) {
	uint8 index = 0;

	// this is not a bag range... using this risks over-writing existing items
	//else if (slot_id >= EmuConstants::BANK_BEGIN && slot_id <= EmuConstants::BANK_END)
	//	index = (slot_id - EmuConstants::BANK_BEGIN) % EmuConstants::ITEM_CONTAINER_SIZE;

	if (slot_id >= invbag::GENERAL_BAGS_BEGIN && slot_id <= invbag::GENERAL_BAGS_END) {
		index = (slot_id - invbag::GENERAL_BAGS_BEGIN) % invbag::SLOT_COUNT;
	}
	else if (slot_id >= invbag::CURSOR_BAG_BEGIN && slot_id <= invbag::CURSOR_BAG_END) {
		index = (slot_id - invbag::CURSOR_BAG_BEGIN); // % invbag::SLOT_COUNT; - not needed since range is 10 slots
	}
	else if (slot_id >= invbag::BANK_BAGS_BEGIN && slot_id <= invbag::BANK_BAGS_END) {
		index = (slot_id - invbag::BANK_BAGS_BEGIN) % invbag::SLOT_COUNT;
	}
	else if (slot_id >= invbag::SHARED_BANK_BAGS_BEGIN && slot_id <= invbag::SHARED_BANK_BAGS_END) {
		index = (slot_id - invbag::SHARED_BANK_BAGS_BEGIN) % invbag::SLOT_COUNT;
	}
	else if (slot_id >= invbag::TRADE_BAGS_BEGIN && slot_id <= invbag::TRADE_BAGS_END) {
		index = (slot_id - invbag::TRADE_BAGS_BEGIN) % invbag::SLOT_COUNT;
	}
	else if (slot_id >= invslot::WORLD_BEGIN && slot_id <= invslot::WORLD_END) {
		index = (slot_id - invslot::WORLD_BEGIN); // % invbag::SLOT_COUNT; - not needed since range is 10 slots
	}

	return index;
}

int16 EQ::InventoryProfile::CalcSlotFromMaterial(uint8 material)
{
	switch (material)
	{
	case textures::armorHead:
		return invslot::slotHead;
	case textures::armorChest:
		return invslot::slotChest;
	case textures::armorArms:
		return invslot::slotArms;
	case textures::armorWrist:
		return invslot::slotWrist1;	// there's 2 bracers, only one bracer material
	case textures::armorHands:
		return invslot::slotHands;
	case textures::armorLegs:
		return invslot::slotLegs;
	case textures::armorFeet:
		return invslot::slotFeet;
	case textures::weaponPrimary:
		return invslot::slotPrimary;
	case textures::weaponSecondary:
		return invslot::slotSecondary;
	default:
		return INVALID_INDEX;
	}
}

uint8 EQ::InventoryProfile::CalcMaterialFromSlot(int16 equipslot)
{
	switch (equipslot)
	{
	case invslot::slotHead:
		return textures::armorHead;
	case invslot::slotChest:
		return textures::armorChest;
	case invslot::slotArms:
		return textures::armorArms;
	case invslot::slotWrist1:
	//case SLOT_BRACER02: // non-live behavior
		return textures::armorWrist;
	case invslot::slotHands:
		return textures::armorHands;
	case invslot::slotLegs:
		return textures::armorLegs;
	case invslot::slotFeet:
		return textures::armorFeet;
	case invslot::slotPrimary:
		return textures::weaponPrimary;
	case invslot::slotSecondary:
		return textures::weaponSecondary;
	default:
		return textures::materialInvalid;
	}
}

bool EQ::InventoryProfile::CanItemFitInContainer(const ItemData *ItemToTry, const ItemData *Container) {

	if (!ItemToTry || !Container)
		return false;

	if (ItemToTry->Size > Container->BagSize)
		return false;

	if ((Container->BagType == item::BagTypeQuiver) && (ItemToTry->ItemType != item::ItemTypeArrow))
		return false;

	if ((Container->BagType == item::BagTypeBandolier) && (ItemToTry->ItemType != item::ItemTypeSmallThrowing))
		return false;

	return true;
}

bool EQ::InventoryProfile::SupportsClickCasting(int16 slot_id)
{
	// there are a few non-potion items that identify as ItemTypePotion..so, we still need to ubiquitously include the equipment range
	if (slot_id >= invslot::EQUIPMENT_BEGIN && slot_id <= invslot::EQUIPMENT_END) {
		return true;
	}
	else if (slot_id >= invslot::GENERAL_BEGIN && slot_id <= invslot::GENERAL_END) {
		return true;
	}
	else if (slot_id >= invbag::GENERAL_BAGS_BEGIN && slot_id <= invbag::GENERAL_BAGS_END) {
		if (inventory::StaticLookup(m_mob_version)->AllowClickCastFromBag)
			return true;
	}

	return false;
}

bool EQ::InventoryProfile::SupportsPotionBeltCasting(int16 slot_id)
{
	// does this have the same criteria as 'SupportsClickCasting' above? (bag clicking per client)
	if (slot_id >= invslot::EQUIPMENT_BEGIN && slot_id <= invslot::EQUIPMENT_END) {
		return true;
	}
	else if (slot_id >= invslot::GENERAL_BEGIN && slot_id <= invslot::GENERAL_END) {
		return true;
	}
	else if (slot_id >= invbag::GENERAL_BAGS_BEGIN && slot_id <= invbag::GENERAL_BAGS_END) {
		return true;
	}

	return false;
}

// Test whether a given slot can support a container item
bool EQ::InventoryProfile::SupportsContainers(int16 slot_id)
{
	if ((slot_id == invslot::slotCursor) ||
		(slot_id >= invslot::GENERAL_BEGIN && slot_id <= invslot::GENERAL_END) ||
		(slot_id >= invslot::BANK_BEGIN && slot_id <= invslot::BANK_END) ||
		(slot_id >= invslot::SHARED_BANK_BEGIN && slot_id <= invslot::SHARED_BANK_END) ||
		(slot_id >= invslot::TRADE_BEGIN && slot_id <= invslot::TRADE_END)
	) {
		return true;
	}

	return false;
}

int EQ::InventoryProfile::GetSlotByItemInst(ItemInstance *inst) {
	if (!inst)
		return INVALID_INDEX;

	int i = GetSlotByItemInstCollection(m_worn, inst);
	if (i != INVALID_INDEX) {
		return i;
	}

	i = GetSlotByItemInstCollection(m_inv, inst);
	if (i != INVALID_INDEX) {
		return i;
	}

	i = GetSlotByItemInstCollection(m_bank, inst);
	if (i != INVALID_INDEX) {
		return i;
	}

	i = GetSlotByItemInstCollection(m_shbank, inst);
	if (i != INVALID_INDEX) {
		return i;
	}

	i = GetSlotByItemInstCollection(m_trade, inst);
	if (i != INVALID_INDEX) {
		return i;
	}

	if (m_cursor.peek_front() == inst) {
		return invslot::slotCursor;
	}

	return INVALID_INDEX;
}

uint8 EQ::InventoryProfile::FindBrightestLightType()
{
	uint8 brightest_light_type = 0;

	for (auto iter = m_worn.begin(); iter != m_worn.end(); ++iter) {
		if ((iter->first < invslot::EQUIPMENT_BEGIN || iter->first > invslot::EQUIPMENT_END))
			continue;

		if (iter->first == invslot::slotAmmo)
			continue;

		auto inst = iter->second;
		if (inst == nullptr)
			continue;

		auto item = inst->GetItem();
		if (item == nullptr)
			continue;

		if (lightsource::IsLevelGreater(item->Light, brightest_light_type))
			brightest_light_type = item->Light;
	}

	uint8 general_light_type = 0;
	for (auto iter = m_inv.begin(); iter != m_inv.end(); ++iter) {
		if (iter->first < invslot::GENERAL_BEGIN || iter->first > invslot::GENERAL_END)
			continue;

		auto inst = iter->second;
		if (inst == nullptr)
			continue;

		auto item = inst->GetItem();
		if (item == nullptr)
			continue;

		if (!item->IsClassCommon())
			continue;
		if (item->Light < 9 || item->Light > 13)
			continue;

		if (lightsource::TypeToLevel(item->Light))
			general_light_type = item->Light;
	}

	if (lightsource::IsLevelGreater(general_light_type, brightest_light_type))
		brightest_light_type = general_light_type;

	return brightest_light_type;
}

void EQ::InventoryProfile::dumpEntireInventory() {

	dumpWornItems();
	dumpInventory();
	dumpBankItems();
	dumpSharedBankItems();

	std::cout << std::endl;
}

void EQ::InventoryProfile::dumpWornItems() {
	std::cout << "Worn items:" << std::endl;
	dumpItemCollection(m_worn);
}

void EQ::InventoryProfile::dumpInventory() {
	std::cout << "Inventory items:" << std::endl;
	dumpItemCollection(m_inv);
}

void EQ::InventoryProfile::dumpBankItems() {

	std::cout << "Bank items:" << std::endl;
	dumpItemCollection(m_bank);
}

void EQ::InventoryProfile::dumpSharedBankItems() {

	std::cout << "Shared Bank items:" << std::endl;
	dumpItemCollection(m_shbank);
}

int EQ::InventoryProfile::GetSlotByItemInstCollection(const std::map<int16, ItemInstance*> &collection, ItemInstance *inst) {
	for (auto iter = collection.begin(); iter != collection.end(); ++iter) {
		ItemInstance *t_inst = iter->second;
		if (t_inst == inst) {
			return iter->first;
		}

		if (t_inst && !t_inst->IsClassBag()) {
			for (auto b_iter = t_inst->_cbegin(); b_iter != t_inst->_cend(); ++b_iter) {
				if (b_iter->second == inst) {
					return InventoryProfile::CalcSlotId(iter->first, b_iter->first);
				}
			}
		}
	}

	return EQ::invslot::SLOT_INVALID;
}

void EQ::InventoryProfile::dumpItemCollection(const std::map<int16, ItemInstance*> &collection)
{
	for (auto it = collection.cbegin(); it != collection.cend(); ++it) {
		auto inst = it->second;
		if (!inst || !inst->GetItem())
			continue;

		std::string slot = StringFormat("Slot %d: %s (%d)", it->first, it->second->GetItem()->Name, (inst->GetCharges() <= 0) ? 1 : inst->GetCharges());
		std::cout << slot << std::endl;

		dumpBagContents(inst, &it);
	}
}

void EQ::InventoryProfile::dumpBagContents(ItemInstance *inst, std::map<int16, ItemInstance*>::const_iterator *it)
{
	if (!inst || !inst->IsClassBag())
		return;

	// Go through bag, if bag
	for (auto itb = inst->_cbegin(); itb != inst->_cend(); ++itb) {
		ItemInstance* baginst = itb->second;
		if (!baginst || !baginst->GetItem())
			continue;

		std::string subSlot = StringFormat("	Slot %d: %s (%d)", InventoryProfile::CalcSlotId((*it)->first, itb->first),
			baginst->GetItem()->Name, (baginst->GetCharges() <= 0) ? 1 : baginst->GetCharges());
		std::cout << subSlot << std::endl;
	}

}

// Internal Method: Retrieves item within an inventory bucket
EQ::ItemInstance* EQ::InventoryProfile::_GetItem(const std::map<int16, ItemInstance*>& bucket, int16 slot_id) const
{
	if (slot_id <= EQ::invslot::POSSESSIONS_END && slot_id >= EQ::invslot::POSSESSIONS_BEGIN) {
		if ((((uint64)1 << slot_id) & m_lookup->PossessionsBitmask) == 0)
			return nullptr;
	}
	else if (slot_id <= EQ::invslot::BANK_END && slot_id >= EQ::invslot::BANK_BEGIN) {
		if (slot_id - EQ::invslot::BANK_BEGIN >= m_lookup->InventoryTypeSize.Bank)
			return nullptr;
	}

	auto it = bucket.find(slot_id);
	if (it != bucket.end()) {
		return it->second;
	}

	// Not found!
	return nullptr;
}

// Internal Method: "put" item into bucket, without regard for what is currently in bucket
// Assumes item has already been allocated
int16 EQ::InventoryProfile::_PutItem(int16 slot_id, ItemInstance* inst)
{
	// What happens here when we _PutItem(MainCursor)? Bad things..really bad things...
	//
	// If putting a nullptr into slot, we need to remove slot without memory delete
	if (inst == nullptr) {
		//Why do we not delete the poped item here????
		PopItem(slot_id);
		return slot_id;
	}

	int16 result = INVALID_INDEX;
	int16 parentSlot = INVALID_INDEX;

	inst->SetEvolveEquipped(false);

	if (slot_id == invslot::slotCursor) {
		// Replace current item on cursor, if exists
		m_cursor.pop(); // no memory delete, clients of this function know what they are doing
		m_cursor.push_front(inst);
		result = slot_id;
	}
	else if (slot_id >= invslot::EQUIPMENT_BEGIN && slot_id <= invslot::EQUIPMENT_END) {
		if ((((uint64)1 << slot_id) & m_lookup->PossessionsBitmask) != 0) {
			if (inst->IsEvolving()) {
				inst->SetEvolveEquipped(true);
			}
			m_worn[slot_id] = inst;
			result = slot_id;
		}
	}
	else if ((slot_id >= invslot::GENERAL_BEGIN && slot_id <= invslot::GENERAL_END)) {
		if ((((uint64)1 << slot_id) & m_lookup->PossessionsBitmask) != 0) {
			m_inv[slot_id] = inst;
			result = slot_id;
		}
	}
	else if (slot_id >= invslot::TRIBUTE_BEGIN && slot_id <= invslot::TRIBUTE_END) {
		m_worn[slot_id] = inst;
		result = slot_id;
	}
	else if (slot_id >= invslot::GUILD_TRIBUTE_BEGIN && slot_id <= invslot::GUILD_TRIBUTE_END) {
		m_worn[slot_id] = inst;
		result = slot_id;
	}
	else if (slot_id >= invslot::BANK_BEGIN && slot_id <= invslot::BANK_END) {
		if (slot_id - EQ::invslot::BANK_BEGIN < m_lookup->InventoryTypeSize.Bank) {
			m_bank[slot_id] = inst;
			result = slot_id;
		}
	}
	else if (slot_id >= invslot::SHARED_BANK_BEGIN && slot_id <= invslot::SHARED_BANK_END) {
		m_shbank[slot_id] = inst;
		result = slot_id;
	}
	else if (slot_id >= invslot::TRADE_BEGIN && slot_id <= invslot::TRADE_END) {
		m_trade[slot_id] = inst;
		result = slot_id;
	}
	else {
		// Slot must be within a bag
		parentSlot = InventoryProfile::CalcSlotId(slot_id);
		ItemInstance* baginst = GetItem(parentSlot); // Get parent bag
		if (baginst && baginst->IsClassBag())
		{
			baginst->_PutItem(InventoryProfile::CalcBagIdx(slot_id), inst);
			result = slot_id;
		}
	}

	if (result == INVALID_INDEX) {
		LogError("Invalid slot_id specified ({}) with parent slot id ({})", slot_id, parentSlot);
		InventoryProfile::MarkDirty(inst); // Slot not found, clean up
	}

	return result;
}

// Internal Method: Checks an inventory bucket for a particular item
int16 EQ::InventoryProfile::_HasItem(std::map<int16, ItemInstance*>& bucket, uint32 item_id, uint8 quantity)
{
	uint32 quantity_found = 0;

	for (auto iter = bucket.begin(); iter != bucket.end(); ++iter) {
		if (iter->first <= EQ::invslot::POSSESSIONS_END && iter->first >= EQ::invslot::POSSESSIONS_BEGIN) {
			if ((((uint64)1 << iter->first) & m_lookup->PossessionsBitmask) == 0)
				continue;
		}
		else if (iter->first <= EQ::invslot::BANK_END && iter->first >= EQ::invslot::BANK_BEGIN) {
			if (iter->first - EQ::invslot::BANK_BEGIN >= m_lookup->InventoryTypeSize.Bank)
				continue;
		}

		auto inst = iter->second;
		if (inst == nullptr) { continue; }

		if (inst->GetID() == item_id) {
			quantity_found += (inst->GetCharges() <= 0) ? 1 : inst->GetCharges();
			if (quantity_found >= quantity)
				return iter->first;
		}

		for (int index = invaug::SOCKET_BEGIN; index <= invaug::SOCKET_END; ++index) {
			if (inst->GetAugmentItemID(index) == item_id && quantity <= 1)
				return invslot::SLOT_AUGMENT_GENERIC_RETURN;
		}

		if (!inst->IsClassBag()) { continue; }

		for (auto bag_iter = inst->_cbegin(); bag_iter != inst->_cend(); ++bag_iter) {
			auto bag_inst = bag_iter->second;
			if (bag_inst == nullptr) { continue; }

			if (bag_inst->GetID() == item_id) {
				quantity_found += (bag_inst->GetCharges() <= 0) ? 1 : bag_inst->GetCharges();
				if (quantity_found >= quantity)
					return InventoryProfile::CalcSlotId(iter->first, bag_iter->first);
			}

			for (int index = invaug::SOCKET_BEGIN; index <= invaug::SOCKET_END; ++index) {
				if (bag_inst->GetAugmentItemID(index) == item_id && quantity <= 1)
					return invslot::SLOT_AUGMENT_GENERIC_RETURN;
			}
		}
	}

	return INVALID_INDEX;
}

// Internal Method: Checks an inventory queue type bucket for a particular item
int16 EQ::InventoryProfile::_HasItem(ItemInstQueue& iqueue, uint32 item_id, uint8 quantity)
{
	// The downfall of this (these) queue procedure is that callers presume that when an item is
	// found, it is presented as being available on the cursor. In cases of a parity check, this
	// is sufficient. However, in cases where referential criteria is considered, this can lead
	// to unintended results. Funtionality should be observed when referencing the return value
	// of this query

	uint32 quantity_found = 0;

	for (auto iter = iqueue.cbegin(); iter != iqueue.cend(); ++iter) {
		auto inst = *iter;
		if (inst == nullptr) { continue; }

		if (inst->GetID() == item_id) {
			quantity_found += (inst->GetCharges() <= 0) ? 1 : inst->GetCharges();
			if (quantity_found >= quantity)
				return invslot::slotCursor;
		}

		for (int index = invaug::SOCKET_BEGIN; index <= invaug::SOCKET_END; ++index) {
			if (inst->GetAugmentItemID(index) == item_id && quantity <= 1)
				return invslot::SLOT_AUGMENT_GENERIC_RETURN;
		}

		if (!inst->IsClassBag()) { continue; }

		for (auto bag_iter = inst->_cbegin(); bag_iter != inst->_cend(); ++bag_iter) {
			auto bag_inst = bag_iter->second;
			if (bag_inst == nullptr) { continue; }

			if (bag_inst->GetID() == item_id) {
				quantity_found += (bag_inst->GetCharges() <= 0) ? 1 : bag_inst->GetCharges();
				if (quantity_found >= quantity)
					return InventoryProfile::CalcSlotId(invslot::slotCursor, bag_iter->first);
			}

			for (int index = invaug::SOCKET_BEGIN; index <= invaug::SOCKET_END; ++index) {
				if (bag_inst->GetAugmentItemID(index) == item_id && quantity <= 1)
					return invslot::SLOT_AUGMENT_GENERIC_RETURN;
			}
		}

		// We only check the visible cursor due to lack of queue processing ability (client allows duplicate in limbo)
		break;
	}

	return INVALID_INDEX;
}

// Internal Method: Checks an inventory bucket for a particular item
int16 EQ::InventoryProfile::_HasItemByUse(std::map<int16, ItemInstance*>& bucket, uint8 use, uint8 quantity)
{
	uint32 quantity_found = 0;

	for (auto iter = bucket.begin(); iter != bucket.end(); ++iter) {
		if (iter->first <= EQ::invslot::POSSESSIONS_END && iter->first >= EQ::invslot::POSSESSIONS_BEGIN) {
			if ((((uint64)1 << iter->first) & m_lookup->PossessionsBitmask) == 0)
				continue;
		}
		else if (iter->first <= EQ::invslot::BANK_END && iter->first >= EQ::invslot::BANK_BEGIN) {
			if (iter->first - EQ::invslot::BANK_BEGIN >= m_lookup->InventoryTypeSize.Bank)
				continue;
		}

		auto inst = iter->second;
		if (inst == nullptr) { continue; }

		if (inst->IsClassCommon() && inst->GetItem()->ItemType == use) {
			quantity_found += (inst->GetCharges() <= 0) ? 1 : inst->GetCharges();
			if (quantity_found >= quantity)
				return iter->first;
		}

		if (!inst->IsClassBag()) { continue; }

		for (auto bag_iter = inst->_cbegin(); bag_iter != inst->_cend(); ++bag_iter) {
			auto bag_inst = bag_iter->second;
			if (bag_inst == nullptr) { continue; }

			if (bag_inst->IsClassCommon() && bag_inst->GetItem()->ItemType == use) {
				quantity_found += (bag_inst->GetCharges() <= 0) ? 1 : bag_inst->GetCharges();
				if (quantity_found >= quantity)
					return InventoryProfile::CalcSlotId(iter->first, bag_iter->first);
			}
		}
	}

	return INVALID_INDEX;
}

// Internal Method: Checks an inventory queue type bucket for a particular item
int16 EQ::InventoryProfile::_HasItemByUse(ItemInstQueue& iqueue, uint8 use, uint8 quantity)
{
	uint32 quantity_found = 0;

	for (auto iter = iqueue.cbegin(); iter != iqueue.cend(); ++iter) {
		auto inst = *iter;
		if (inst == nullptr) { continue; }

		if (inst->IsClassCommon() && inst->GetItem()->ItemType == use) {
			quantity_found += (inst->GetCharges() <= 0) ? 1 : inst->GetCharges();
			if (quantity_found >= quantity)
				return invslot::slotCursor;
		}

		if (!inst->IsClassBag()) { continue; }

		for (auto bag_iter = inst->_cbegin(); bag_iter != inst->_cend(); ++bag_iter) {
			auto bag_inst = bag_iter->second;
			if (bag_inst == nullptr) { continue; }

			if (bag_inst->IsClassCommon() && bag_inst->GetItem()->ItemType == use) {
				quantity_found += (bag_inst->GetCharges() <= 0) ? 1 : bag_inst->GetCharges();
				if (quantity_found >= quantity)
					return InventoryProfile::CalcSlotId(invslot::slotCursor, bag_iter->first);
			}
		}

		// We only check the visible cursor due to lack of queue processing ability (client allows duplicate in limbo)
		break;
	}

	return INVALID_INDEX;
}

int16 EQ::InventoryProfile::_HasItemByLoreGroup(std::map<int16, ItemInstance*>& bucket, uint32 loregroup)
{
	for (auto iter = bucket.begin(); iter != bucket.end(); ++iter) {
		if (iter->first <= EQ::invslot::POSSESSIONS_END && iter->first >= EQ::invslot::POSSESSIONS_BEGIN) {
			if ((((uint64)1 << iter->first) & m_lookup->PossessionsBitmask) == 0)
				continue;
		}
		else if (iter->first <= EQ::invslot::BANK_END && iter->first >= EQ::invslot::BANK_BEGIN) {
			if (iter->first - EQ::invslot::BANK_BEGIN >= m_lookup->InventoryTypeSize.Bank)
				continue;
		}

		auto inst = iter->second;
		if (inst == nullptr) { continue; }

		if (inst->GetItem()->LoreGroup == loregroup)
			return iter->first;

		for (int index = invaug::SOCKET_BEGIN; index <= invaug::SOCKET_END; ++index) {
			auto aug_inst = inst->GetAugment(index);
			if (aug_inst == nullptr) { continue; }

			if (aug_inst->GetItem()->LoreGroup == loregroup)
				return invslot::SLOT_AUGMENT_GENERIC_RETURN;
		}

		if (!inst->IsClassBag()) { continue; }

		for (auto bag_iter = inst->_cbegin(); bag_iter != inst->_cend(); ++bag_iter) {
			auto bag_inst = bag_iter->second;
			if (bag_inst == nullptr) { continue; }

			if (bag_inst->IsClassCommon() && bag_inst->GetItem()->LoreGroup == loregroup)
				return InventoryProfile::CalcSlotId(iter->first, bag_iter->first);

			for (int index = invaug::SOCKET_BEGIN; index <= invaug::SOCKET_END; ++index) {
				auto aug_inst = bag_inst->GetAugment(index);
				if (aug_inst == nullptr) { continue; }

				if (aug_inst->GetItem()->LoreGroup == loregroup)
					return invslot::SLOT_AUGMENT_GENERIC_RETURN;
			}
		}
	}

	return EQ::invslot::SLOT_INVALID;
}

// Internal Method: Checks an inventory queue type bucket for a particular item
int16 EQ::InventoryProfile::_HasItemByLoreGroup(ItemInstQueue& iqueue, uint32 loregroup)
{
	for (auto iter = iqueue.cbegin(); iter != iqueue.cend(); ++iter) {
		auto inst = *iter;
		if (inst == nullptr) { continue; }

		if (inst->GetItem()->LoreGroup == loregroup)
			return invslot::slotCursor;

		for (int index = invaug::SOCKET_BEGIN; index <= invaug::SOCKET_END; ++index) {
			auto aug_inst = inst->GetAugment(index);
			if (aug_inst == nullptr) { continue; }

			if (aug_inst->GetItem()->LoreGroup == loregroup)
				return invslot::SLOT_AUGMENT_GENERIC_RETURN;
		}

		if (!inst->IsClassBag()) { continue; }

		for (auto bag_iter = inst->_cbegin(); bag_iter != inst->_cend(); ++bag_iter) {
			auto bag_inst = bag_iter->second;
			if (bag_inst == nullptr) { continue; }

			if (bag_inst->IsClassCommon() && bag_inst->GetItem()->LoreGroup == loregroup)
				return InventoryProfile::CalcSlotId(invslot::slotCursor, bag_iter->first);

			for (int index = invaug::SOCKET_BEGIN; index <= invaug::SOCKET_END; ++index) {
				auto aug_inst = bag_inst->GetAugment(index);
				if (aug_inst == nullptr) { continue; }

				if (aug_inst->GetItem()->LoreGroup == loregroup)
					return invslot::SLOT_AUGMENT_GENERIC_RETURN;
			}
		}

		// We only check the visible cursor due to lack of queue processing ability (client allows duplicate in limbo)
		break;
	}

	return EQ::invslot::SLOT_INVALID;
}

std::vector<uint32> EQ::InventoryProfile::GetAugmentIDsBySlotID(int16 slot_id)
{
	std::vector<uint32> augments;
	const auto* item = GetItem(slot_id);

	if (item) {
		for (uint8 i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; i++) {
			augments.push_back(item->GetAugment(i) ? item->GetAugmentItemID(i) : 0);
		}
	}

	return augments;
}

std::vector<int16> EQ::InventoryProfile::FindAllFreeSlotsThatFitItem(const EQ::ItemData *item_data)
{
	std::vector<int16> free_slots{};
	for (int16         i = EQ::invslot::GENERAL_BEGIN; i <= EQ::invslot::GENERAL_END; i++) {
		if ((((uint64) 1 << i) & GetLookup()->PossessionsBitmask) == 0) {
			continue;
		}

		EQ::ItemInstance *inv_item = GetItem(i);

		if (!inv_item) {
			// Found available slot in personal inventory
			free_slots.push_back(i);
		}

		if (inv_item->IsClassBag() &&
			EQ::InventoryProfile::CanItemFitInContainer(item_data, inv_item->GetItem())) {

			int16 base_slot_id = EQ::InventoryProfile::CalcSlotId(i, EQ::invbag::SLOT_BEGIN);
			uint8 bag_size     = inv_item->GetItem()->BagSlots;

			for (uint8 bag_slot = EQ::invbag::SLOT_BEGIN; bag_slot < bag_size; bag_slot++) {
				auto bag_item = GetItem(base_slot_id + bag_slot);
				if (!bag_item) {
					// Found available slot within bag
					free_slots.push_back(i);
				}
			}
		}
	}
	return free_slots;
}

int16 EQ::InventoryProfile::FindFirstFreeSlotThatFitsItem(const EQ::ItemData *item_data)
{
	for (int16         i = EQ::invslot::GENERAL_BEGIN; i <= EQ::invslot::GENERAL_END; i++) {
		if ((((uint64) 1 << i) & GetLookup()->PossessionsBitmask) == 0) {
			continue;
		}

		EQ::ItemInstance *inv_item = GetItem(i);

		if (!inv_item) {
			// Found available slot in personal inventory
			return i;
		}

		if (inv_item->IsClassBag() &&
			EQ::InventoryProfile::CanItemFitInContainer(item_data, inv_item->GetItem())) {

			int16 base_slot_id = EQ::InventoryProfile::CalcSlotId(i, EQ::invbag::SLOT_BEGIN);
			uint8 bag_size     = inv_item->GetItem()->BagSlots;

			for (uint8 bag_slot = EQ::invbag::SLOT_BEGIN; bag_slot < bag_size; bag_slot++) {
				auto bag_item = GetItem(base_slot_id + bag_slot);
				if (!bag_item) {
					// Found available slot within bag
					return base_slot_id + bag_slot;
				}
			}
		}
	}
	return 0;
}

//This function has the same flaw as noted above
// Helper functions for evolving items
int16 EQ::InventoryProfile::HasEvolvingItem(uint64 evolve_unique_id, uint8 quantity, uint8 where)
{
	int16 slot_id = INVALID_INDEX;

	// Altered by Father Nitwit to support a specification of
	// where to search, with a default value to maintain compatibility

	// Check each inventory bucket
	if (where & invWhereWorn) {
		slot_id = _HasEvolvingItem(m_worn, evolve_unique_id, quantity);
		if (slot_id != INVALID_INDEX) {
			return slot_id;
		}
	}

	if (where & invWherePersonal) {
		slot_id = _HasEvolvingItem(m_inv, evolve_unique_id, quantity);
		if (slot_id != INVALID_INDEX) {
			return slot_id;
		}
	}

	if (where & invWhereBank) {
		slot_id = _HasEvolvingItem(m_bank, evolve_unique_id, quantity);
		if (slot_id != INVALID_INDEX) {
			return slot_id;
		}
	}

	if (where & invWhereSharedBank) {
		slot_id = _HasEvolvingItem(m_shbank, evolve_unique_id, quantity);
		if (slot_id != INVALID_INDEX) {
			return slot_id;
		}
	}

	if (where & invWhereTrading) {
		slot_id = _HasEvolvingItem(m_trade, evolve_unique_id, quantity);
		if (slot_id != INVALID_INDEX) {
			return slot_id;
		}
	}

	// Behavioral change - Limbo is no longer checked due to improper handling of return value
	if (where & invWhereCursor) {
		// Check cursor queue
		slot_id = _HasEvolvingItem(m_cursor, evolve_unique_id, quantity);
		if (slot_id != INVALID_INDEX) {
			return slot_id;
		}
	}

	return slot_id;
}

// Internal Method: Checks an inventory bucket for a particular evolving item unique id
int16 EQ::InventoryProfile::_HasEvolvingItem(
	std::map<int16, ItemInstance *> &bucket, uint64 evolve_unique_id, uint8 quantity)
{
	uint32 quantity_found = 0;

	for (auto const &[key, value]: bucket) {
		if (!value) {
			continue;
		}

		if (key <= EQ::invslot::POSSESSIONS_END && key >= EQ::invslot::POSSESSIONS_BEGIN) {
			if (((uint64) 1 << key & m_lookup->PossessionsBitmask) == 0) {
				continue;
			}
		}
		else if (key <= EQ::invslot::BANK_END && key >= EQ::invslot::BANK_BEGIN) {
			if (key - EQ::invslot::BANK_BEGIN >= m_lookup->InventoryTypeSize.Bank) {
				continue;
			}
		}


		if (value->GetEvolveUniqueID() == evolve_unique_id) {
			quantity_found += value->GetCharges() <= 0 ? 1 : value->GetCharges();
			if (quantity_found >= quantity) {
				return key;
			}
		}

		for (int index = invaug::SOCKET_BEGIN; index <= invaug::SOCKET_END; ++index) {
			if (value->GetAugmentEvolveUniqueID(index) == evolve_unique_id && quantity <= 1) {
				return invslot::SLOT_AUGMENT_GENERIC_RETURN;
			}
		}

		if (!value->IsClassBag()) {
			continue;
		}

		for (auto const &[bag_key, bag_value]: *value->GetContents()) {
			if (!bag_value) {
				continue;
			}

			if (bag_value->GetEvolveUniqueID() == evolve_unique_id) {
				quantity_found += bag_value->GetCharges() <= 0 ? 1 : bag_value->GetCharges();
				if (quantity_found >= quantity) {
					return CalcSlotId(key, bag_key);
				}
			}

			for (int index = invaug::SOCKET_BEGIN; index <= invaug::SOCKET_END; ++index) {
				if (bag_value->GetAugmentEvolveUniqueID(index) == evolve_unique_id && quantity <= 1) {
					return invslot::SLOT_AUGMENT_GENERIC_RETURN;
				}
			}
		}
	}

	return INVALID_INDEX;
}

// Internal Method: Checks an inventory queue type bucket for a particular item
int16 EQ::InventoryProfile::_HasEvolvingItem(ItemInstQueue &iqueue, uint64 evolve_unique_id, uint8 quantity)
{
	// The downfall of this (these) queue procedure is that callers presume that when an item is
	// found, it is presented as being available on the cursor. In cases of a parity check, this
	// is sufficient. However, in cases where referential criteria is considered, this can lead
	// to unintended results. Funtionality should be observed when referencing the return value
	// of this query

	uint32 quantity_found = 0;

	for (auto const &inst: iqueue) {
		if (!inst) {
			continue;
		}

		if (inst->GetEvolveUniqueID() == evolve_unique_id) {
			quantity_found += inst->GetCharges() <= 0 ? 1 : inst->GetCharges();
			if (quantity_found >= quantity) {
				return invslot::slotCursor;
			}
		}

		for (int index = invaug::SOCKET_BEGIN; index <= invaug::SOCKET_END; ++index) {
			if (inst->GetAugmentEvolveUniqueID(index) == evolve_unique_id && quantity <= 1) {
				return invslot::SLOT_AUGMENT_GENERIC_RETURN;
			}
		}

		if (!inst->IsClassBag()) {
			continue;
		}

		for (auto const &[bag_key, bag_value]: *inst->GetContents()) {
			if (!bag_value) {
				continue;
			}

			if (bag_value->GetEvolveUniqueID() == evolve_unique_id) {
				quantity_found += bag_value->GetCharges() <= 0 ? 1 : bag_value->GetCharges();
				if (quantity_found >= quantity) {
					return CalcSlotId(invslot::slotCursor, bag_key);
				}
			}

			for (int index = invaug::SOCKET_BEGIN; index <= invaug::SOCKET_END; ++index) {
				if (bag_value->GetAugmentEvolveUniqueID(index) == evolve_unique_id && quantity <= 1) {
					return invslot::SLOT_AUGMENT_GENERIC_RETURN;
				}
			}
		}

		// We only check the visible cursor due to lack of queue processing ability (client allows duplicate in limbo)
		break;
	}

	return INVALID_INDEX;
}
