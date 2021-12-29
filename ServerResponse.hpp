#pragma once
#include <iostream>
#include "nlohmann/json.hpp"
#include "ItemInfo.hpp"

class cServerResponse
{
private:
  std::string getitemstatestr()
  {
      switch(iitemstate)
      {
          case ItemState::none:
          return "none";
          case ItemState::queued:
          return "queued";
          case ItemState::completed:
          return "completed";
          case ItemState::failed:
          return "failed";
      }
      return "";
  }

public:
    std::string assetid="";
    bool success=false;
    std::string itemstate = "none";
    int iitemstate = ItemState::none;
    std::string errmessage="";
    std::string exmessage="";


    std::string toJsonStr()
    {
        nlohmann::json j = nlohmann::json
        {
            {"assetid", this->assetid},
            {"success", this->success},
            {"itemstate", this->getitemstatestr()},
            {"iitemstate", this->iitemstate},
            {"errmessage", this->errmessage},
            {"exmessage", this->exmessage}
        };
        return j.dump();
    }
};