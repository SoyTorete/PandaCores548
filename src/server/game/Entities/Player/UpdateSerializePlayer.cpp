/*
 * Copyright (C) 2005-2016 Uwow <http://uwow.biz/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Containers.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Item.h"
#include "Group.h"
#include "Spell.h"
#include "Util.h"
#include "World.h"
#include "WorldSession.h"
#include "InstanceSaveMgr.h"
#include "InstanceScript.h"
#include "BattlePetMgr.h"
#include "ReputationMgr.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "AuctionHouseMgr.h"
#include "GameEventMgr.h"
#include "SocialMgr.h"
#include "SerializePlayer.h"
#include "AchievementMgr.h"

void Player::UpdateSerializePlayer()
{
    SerializePlayer();
    SerializePlayerGroup();
    SerializePlayerAuras();
}

void Player::DeleteSerializePlayerCriteriaProgress(AchievementEntry const* achievement)
{
    std::string achievID = std::to_string(achievement->ID);
    if (achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT)
    {
        Json::Value CriteriaAc;
        CriteriaAc = true;
        AccountCriteriaJson[achievID.c_str()] = CriteriaAc;
        RedisDatabase.AsyncExecuteHSet("HSET", criteriaAcKey, achievID.c_str(), jsonBuilder.write(CriteriaAc).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "UpdateSerializePlayerCriteriaProgress account guid %u", guid);
        });
    }
    else
    {
        Json::Value CriteriaPl;
        CriteriaPl = true;
        PlayerCriteriaJson[achievID.c_str()] = CriteriaPl;
        RedisDatabase.AsyncExecuteHSet("HSET", criteriaPlKey, achievID.c_str(), jsonBuilder.write(CriteriaPl).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
            sLog->outInfo(LOG_FILTER_REDIS, "UpdateSerializePlayerCriteriaProgress player guid %u", guid);
        });
    }
}

void Player::UpdateSerializePlayerCriteriaProgress(AchievementEntry const* achievement, CriteriaProgressMap* progressMap)
{
    std::string achievID = std::to_string(achievement->ID);
    Json::Value CriteriaPl;
    Json::Value CriteriaAc;

    for (auto iter = progressMap->begin(); iter != progressMap->end(); ++iter)
    {
        if (iter->second.deactiveted)
            continue;

        // store data only for real progress
        if (iter->second.counter != 0)
        {
            std::string criteriaId = std::to_string(iter->first);
            if (achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT)
            {
                CriteriaAc[criteriaId.c_str()]["counter"] = iter->second.counter;
                CriteriaAc[criteriaId.c_str()]["date"] = iter->second.date;
                CriteriaAc[criteriaId.c_str()]["completed"] = iter->second.completed;
            }
            else
            {
                CriteriaPl[criteriaId.c_str()]["counter"] = iter->second.counter;
                CriteriaPl[criteriaId.c_str()]["date"] = iter->second.date;
                CriteriaPl[criteriaId.c_str()]["completed"] = iter->second.completed;
            }
        }
    }

    if (achievement->flags & ACHIEVEMENT_FLAG_ACCOUNT)
    {
        AccountCriteriaJson[achievID.c_str()] = CriteriaAc;
        RedisDatabase.AsyncExecuteHSet("HSET", criteriaAcKey, achievID.c_str(), jsonBuilder.write(CriteriaAc).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
            //sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerCriteria account guid %u", guid);
        });
    }
    else
    {
        PlayerCriteriaJson[achievID.c_str()] = CriteriaPl;
        RedisDatabase.AsyncExecuteHSet("HSET", criteriaPlKey, achievID.c_str(), jsonBuilder.write(CriteriaPl).c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
            //sLog->outInfo(LOG_FILTER_REDIS, "Player::SerializePlayerCriteria player guid %u", guid);
        });
    }
}

void AuctionEntry::UpdateSerializeAuction()
{
    AuctionJson["Id"] = Id;
    AuctionJson["auctioneer"] = auctioneer;
    AuctionJson["itemGUIDLow"] = itemGUIDLow;
    AuctionJson["itemEntry"] = itemEntry;
    AuctionJson["itemCount"] = itemCount;
    AuctionJson["owner"] = owner;
    AuctionJson["startbid"] = startbid;
    AuctionJson["bid"] = bid;
    AuctionJson["buyout"] = buyout;
    AuctionJson["expire_time"] = expire_time;
    AuctionJson["bidder"] = bidder;
    AuctionJson["deposit"] = deposit;
    AuctionJson["factionTemplateId"] = factionTemplateId;
}

void AuctionEntry::DeleteFromRedis()
{
    std::string index = std::to_string(Id);

    RedisDatabase.AsyncExecuteH("HDEL", auctionKey, index.c_str(), Id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Item::DeleteFromRedis guid %u", guid);
    });
}

void Player::RemoveMailFromRedis(uint32 id)
{
    std::string index = std::to_string(id);
    RedisDatabase.AsyncExecuteH("HDEL", mailKey, index.c_str(), id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::RemoveMailFromRedis id %u", guid);
    });
}

void Player::RemoveMailItemsFromRedis(uint32 id)
{
    char* _key = new char[32];
    sprintf(_key, "r{%u}m{%u}items", realmID, id);

    RedisDatabase.AsyncExecute("DEL", _key, id, [&](const RedisValue &v, uint64 guid) {
        sLog->outInfo(LOG_FILTER_REDIS, "Player::RemoveMailItemsFromRedis id %u", guid);
    });
}