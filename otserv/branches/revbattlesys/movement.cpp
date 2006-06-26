//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////


#include "creature.h"
#include "player.h"
#include "tile.h"
#include <sstream>
#include "tools.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h> 

#include "movement.h"

extern Game g_game;

MoveEvents::MoveEvents() :
m_scriptInterface("MoveEvents Interface")
{
	m_scriptInterface.initState();
}

MoveEvents::~MoveEvents()
{
	clear();
}

void MoveEvents::clear()
{
	MoveListMap::iterator it = m_itemIdMap.begin();
	while(it != m_itemIdMap.end()){
		for(int i = 0; i < MOVE_EVENT_LAST; ++i){
			delete it->second.event[i];
		}
		m_itemIdMap.erase(it);
		it = m_itemIdMap.begin();
	}
	
	it = m_actionIdMap.begin();
	while(it != m_actionIdMap.end()){
		for(int i = 0; i < MOVE_EVENT_LAST; ++i){
			delete it->second.event[i];
		}
		m_actionIdMap.erase(it);
		it = m_actionIdMap.begin();
	}
	
	it = m_uniqueIdMap.begin();
	while(it != m_uniqueIdMap.end()){
		for(int i = 0; i < MOVE_EVENT_LAST; ++i){
			delete it->second.event[i];
		}
		m_uniqueIdMap.erase(it);
		it = m_uniqueIdMap.begin();
	}
	
	m_scriptInterface.reInitState();
}


LuaScriptInterface& MoveEvents::getScriptInterface()
{
	return m_scriptInterface;	
}	
std::string MoveEvents::getScriptBaseName()
{
	return "movements";	
}
	
bool MoveEvents::registerEvent(Event* event, xmlNodePtr p)
{
	MoveEvent* moveEvent = dynamic_cast<MoveEvent*>(event);
	if(!moveEvent)
		return false;
		
	bool success = true;
	int id;
	if(readXMLInteger(p,"itemid",id)){
		addEvent(moveEvent, id, m_itemIdMap);
	}
	else if(readXMLInteger(p,"uniqueid",id)){
		addEvent(moveEvent, id, m_uniqueIdMap);
	}
	else if(readXMLInteger(p,"actionid",id)){
		addEvent(moveEvent, id, m_actionIdMap);
	}
	else{
		//TODO, position
		success = false;
	}
	return success;
}
	
Event* MoveEvents::getEvent(const std::string& nodeName)
{
	if(nodeName == "movevent"){
		return new MoveEvent(&m_scriptInterface);
	}
	else{
		return NULL;
	}
}	

void MoveEvents::addEvent(MoveEvent* event, long id, MoveListMap& map)
{
	MoveListMap::iterator it = map.find(id);
	if(it == map.end()){
		MoveEventList moveEventList;
		moveEventList.event[event->getEventType()] = event;
		map[id] = moveEventList;
	}
	else{
		it->second.event[event->getEventType()] = event;
	}
	return;
}

MoveEvent* MoveEvents::getEvent(Item* item, MoveEvent_t eventType)
{
	MoveListMap::iterator it;
	if(item->getUniqueId() != 0){
		it = m_uniqueIdMap.find(item->getUniqueId());
    	if(it != m_uniqueIdMap.end()){
			return it->second.event[eventType];
		}
	}
	if(item->getActionId() != 0){
		it = m_actionIdMap.find(item->getActionId());
    	if (it != m_actionIdMap.end()){
	    	return it->second.event[eventType];
		}
	}
	
	it = m_itemIdMap.find(item->getID());
    if (it != m_itemIdMap.end()){
	   	return it->second.event[eventType];
	}
	
	return NULL;
}

long MoveEvents::onCreatureMove(Creature* creature, Tile* tile, bool isIn)
{
	MoveEvent_t eventType;
	if(isIn){
		eventType = MOVE_EVENT_STEP_IN;
	}
	else{
		eventType = MOVE_EVENT_STEP_OUT;
	}
	
	long j = tile->__getLastIndex();
	for(long i = tile->__getFirstIndex(); i < j; ++i){
		Thing* thing = tile->__getThing(i);
		if(Item* item = thing->getItem()){
			MoveEvent* event = getEvent(item, eventType);
			if(event){
				return event->executeStep(creature, item, tile->getPosition());
			}
		}
	}
	return -1;
}

long MoveEvents::onPlayerEquip(Player* player, Item* item, long slot, bool isEquip)
{
	MoveEvent_t eventType;
	if(isEquip){
		eventType = MOVE_EVENT_EQUIP;
	}
	else{
		eventType = MOVE_EVENT_DEEQUIP;
	}
	
	MoveEvent* event = getEvent(item, eventType);
	if(event){
		return event->executeEquip(player, item, slot);
	}
	return -1;
}

long MoveEvents::onItemMove(Creature* creature, Item* item, Tile* tile, bool isAdd)
{
	MoveEvent_t eventType;
	if(isAdd){
		eventType = MOVE_EVENT_ADD_ITEM;
	}
	else{
		eventType = MOVE_EVENT_REMOVE_ITEM;
	}
	
	MoveEvent* event = getEvent(item, eventType);
	if(event){
		return event->executeAddRemItem(creature, item, tile->getPosition());
	}
	return -1;
}


MoveEvent::MoveEvent(LuaScriptInterface* _interface) :
Event(_interface)
{
	m_eventType = MOVE_EVENT_NONE;
}

MoveEvent::~MoveEvent()
{
	//
}

std::string MoveEvent::getScriptEventName()
{
	switch(m_eventType){
	case MOVE_EVENT_STEP_IN:
		return "onStepIn";
		break;
	case MOVE_EVENT_STEP_OUT:
		return "onStepOut";
		break;
	case MOVE_EVENT_EQUIP:
		return "onEquip";
		break;
	case MOVE_EVENT_DEEQUIP:
		return "onDeEquip";
		break;
	case MOVE_EVENT_ADD_ITEM:
		return "onAddItem";
		break;
	case MOVE_EVENT_REMOVE_ITEM:
		return "onRemoveItem";
		break;
	default:
		std::cout << "Error: [MoveEvent::getScriptEventName()] No valid event type." <<  std::endl;
		return "";
		break;
	};
}

bool MoveEvent::configureEvent(xmlNodePtr p)
{
	std::string str;
	if(readXMLString(p, "event", str)){
		if(str == "StepIn"){
			m_eventType = MOVE_EVENT_STEP_IN;
		}
		else if(str == "StepOut"){
			m_eventType = MOVE_EVENT_STEP_OUT;
		}
		else if(str == "Equip"){
			m_eventType = MOVE_EVENT_EQUIP;
		}
		else if(str == "DeEquip"){
			m_eventType = MOVE_EVENT_DEEQUIP;
		}
		else if(str == "AddItem"){
			m_eventType = MOVE_EVENT_ADD_ITEM;
		}
		else if(str == "RemoveItem"){
			m_eventType = MOVE_EVENT_REMOVE_ITEM;
		}
		else{
			std::cout << "Error: [MoveEvent::configureMoveEvent] No valid event name " << str << std::endl;
			return false;
		}
	}
	else{
		std::cout << "Error: [MoveEvent::configureMoveEvent] No event found." << std::endl;
		return false;
	}
	return true;
}

MoveEvent_t MoveEvent::getEventType() const
{
	if(m_eventType == MOVE_EVENT_NONE){
		std::cout << "Error: [MoveEvent::getEventType()] MOVE_EVENT_NONE" << std::endl;
		return (MoveEvent_t)0;
	}
	return m_eventType;
}

long MoveEvent::executeStep(Creature* creature, Item* item, const Position& pos)
{
	//onStepIn(cid, item, pos)
	//onStepOut(cid, item, pos)
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
	//debug only
	std::stringstream desc;
	desc << creature->getName() << " itemid: " << item->getID() << " - " << pos;
	env->setEventDesc(desc.str());
	//
	
	env->setScriptId(m_scriptId, m_scriptInterface);
	env->setRealPos(pos);
	
	long cid = env->addThing((Thing*)creature);
	long itemid = env->addThing(item);
	
	lua_State* L = m_scriptInterface->getLuaState();
	
	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);
	LuaScriptInterface::pushThing(L, item, itemid);
	LuaScriptInterface::pushPosition(L, pos, 0);
	
	long ret;
	if(m_scriptInterface->callFunction(3, ret) == false){
		ret = 0;
	}
	
	return ret;
}

long MoveEvent::executeEquip(Player* player, Item* item, long slot)
{
	//onEquip(cid, item, slot)
	//onStepOut(cid, item, slot)
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
	//debug only
	std::stringstream desc;
	desc << player->getName() << " itemid:" << item->getID() << " slot:" << slot;
	env->setEventDesc(desc.str());
	//
	
	env->setScriptId(m_scriptId, m_scriptInterface);
	env->setRealPos(player->getPosition());
	
	long cid = env->addThing((Thing*)player);
	long itemid = env->addThing(item);
	
	lua_State* L = m_scriptInterface->getLuaState();
	
	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);
	LuaScriptInterface::pushThing(L, item, itemid);
	lua_pushnumber(L, slot);
	
	long ret;
	if(m_scriptInterface->callFunction(3, ret) == false){
		ret = 0;
	}
	
	return ret;
}

long MoveEvent::executeAddRemItem(Creature* creature, Item* item, const Position& pos)
{
	//onAddItem(cid, item, pos)
	//onRemoveItem(cid, item, pos)
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
	//debug only
	std::stringstream desc;
	desc << creature->getName() << " itemid: " << item->getID() << " - " << pos;
	env->setEventDesc(desc.str());
	//
	
	env->setScriptId(m_scriptId, m_scriptInterface);
	env->setRealPos(pos);
	
	long cid = env->addThing((Thing*)creature);
	long itemid = env->addThing(item);
	
	lua_State* L = m_scriptInterface->getLuaState();
	
	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);
	LuaScriptInterface::pushThing(L, item, itemid);
	LuaScriptInterface::pushPosition(L, pos, 0);
	
	long ret;
	if(m_scriptInterface->callFunction(3, ret) == false){
		ret = 0;
	}
	
	return ret;
}
