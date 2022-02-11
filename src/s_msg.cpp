/* s_msg.c
 *
 * Micropolis, Unix Version.  This game was released for the Unix platform
 * in or about 1990 and has been modified for inclusion in the One Laptop
 * Per Child program.  Copyright (C) 1989 - 2007 Electronic Arts Inc.  If
 * you need assistance with this program, you may contact:
 *   http://wiki.laptop.org/go/Micropolis  or email  micropolis@laptop.org.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.  You should have received a
 * copy of the GNU General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 * 
 *             ADDITIONAL TERMS per GNU GPL Section 7
 * 
 * No trademark or publicity rights are granted.  This license does NOT
 * give you any right, title or interest in the trademark SimCity or any
 * other Electronic Arts trademark.  You may not distribute any
 * modification of this program using the trademark SimCity or claim any
 * affliation or association with Electronic Arts Inc. or its employees.
 * 
 * Any propagation or conveyance of this program must include this
 * copyright notice and these terms.
 * 
 * If you convey this program (or any modifications of it) and assume
 * contractual liability for the program to recipients of it, you agree
 * to indemnify Electronic Arts for any liability that those contractual
 * assumptions impose on Electronic Arts.
 * 
 * You may not misrepresent the origins of this program; modified
 * versions of the program must be marked as such and not identified as
 * the original program.
 * 
 * This disclaimer supplements the one included in the General Public
 * License.  TO THE FULLEST EXTENT PERMISSIBLE UNDER APPLICABLE LAW, THIS
 * PROGRAM IS PROVIDED TO YOU "AS IS," WITH ALL FAULTS, WITHOUT WARRANTY
 * OF ANY KIND, AND YOUR USE IS AT YOUR SOLE RISK.  THE ENTIRE RISK OF
 * SATISFACTORY QUALITY AND PERFORMANCE RESIDES WITH YOU.  ELECTRONIC ARTS
 * DISCLAIMS ANY AND ALL EXPRESS, IMPLIED OR STATUTORY WARRANTIES,
 * INCLUDING IMPLIED WARRANTIES OF MERCHANTABILITY, SATISFACTORY QUALITY,
 * FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS, AND WARRANTIES (IF ANY) ARISING FROM A COURSE OF DEALING,
 * USAGE, OR TRADE PRACTICE.  ELECTRONIC ARTS DOES NOT WARRANT AGAINST
 * INTERFERENCE WITH YOUR ENJOYMENT OF THE PROGRAM; THAT THE PROGRAM WILL
 * MEET YOUR REQUIREMENTS; THAT OPERATION OF THE PROGRAM WILL BE
 * UNINTERRUPTED OR ERROR-FREE, OR THAT THE PROGRAM WILL BE COMPATIBLE
 * WITH THIRD PARTY SOFTWARE OR THAT ANY ERRORS IN THE PROGRAM WILL BE
 * CORRECTED.  NO ORAL OR WRITTEN ADVICE PROVIDED BY ELECTRONIC ARTS OR
 * ANY AUTHORIZED REPRESENTATIVE SHALL CREATE A WARRANTY.  SOME
 * JURISDICTIONS DO NOT ALLOW THE EXCLUSION OF OR LIMITATIONS ON IMPLIED
 * WARRANTIES OR THE LIMITATIONS ON THE APPLICABLE STATUTORY RIGHTS OF A
 * CONSUMER, SO SOME OR ALL OF THE ABOVE EXCLUSIONS AND LIMITATIONS MAY
 * NOT APPLY TO YOU.
 */
#include "main.h"

#include "s_sim.h"

#include "w_resrc.h"
#include "w_sound.h"
#include "w_stubs.h"
#include "w_tk.h"


#include "Point.h"

#include <algorithm>
#include <string>


namespace
{
    int LastCityPop{};
    int LastPictureId{};

    MessageEnumerator messageId{};
    MessageEnumerator LastCategory{};

    bool AutoGotoLocation{ false };

    Point<int> messageLocation{};
    std::string lastMessage;
};


void AutoGotoMessageLocation(bool autogo)
{
    AutoGotoLocation = autogo;
}


bool AutoGotoMessageLocation()
{
    return AutoGotoLocation;
}


void LastMessage(const std::string& message)
{
    lastMessage = message;
}


const std::string& LastMessage()
{
    return lastMessage;
}


MessageEnumerator MessageId()
{
    return messageId;
}


void MessageId(MessageEnumerator id)
{
    messageId = id;
}


void MessageLocation(Point<int> location)
{
    messageLocation = location;
}


const Point<int>& MessageLocation()
{
    return messageLocation;
}


void ClearMes()
{
    //MessagePort = 0;
    MessageId(MessageEnumerator::None);
    MessageLocation({ 0, 0 });
    LastPictureId = 0;
}


int SendMes(MessageEnumerator id)
{
    MessageId(id);
    return 0;
}


void SendMesAt(MessageEnumerator id, int x, int y)
{
    if (SendMes(id))
    {
        MessageLocation({ x, y });
    }
}


void SetMessageField(const std::string& msg)
{
    if (LastMessage() != msg)
    {
        LastMessage(msg);
        Eval(std::string("UISetMessage {" + msg + "}").c_str());
    }
}


void DoAutoGoto(int x, int y, const std::string& msg)
{
    SetMessageField(msg);
    Eval(std::string("UIAutoGoto " + std::to_string(x) + " " + std::to_string(y)).c_str());
}


void DoShowPicture(int id)
{
    Eval(std::string("UIShowPicture " + std::to_string(id)).c_str());
}


void DoLoseGame()
{
    Eval("UILoseGame");
}


void DoWinGame()
{
    Eval("UIWinGame");
}


void DoScenarioScore(int type)
{
    int z;

    z = -200;	/* you lose */
    switch (type)
    {
    case 1:	/* Dullsville */
        if (CityClass >= 4)
        {
            z = -100;
        }
        break;

    case 2:	/* San Francisco */
        if (CityClass >= 4)
        {
            z = -100;
        }
        break;

    case 3:	/* Hamburg */
        if (CityClass >= 4)
        {
            z = -100;
        }
        break;

    case 4:	/* Bern */
        if (TrafficAverage < 80)
        {
            z = -100;
        }
        break;

    case 5:	/* Tokyo */
        if (CityScore > 500)
        {
            z = -100;
        }
        break;

    case 6:	/* Detroit */
        if (CrimeAverage < 60)
        {
            z = -100;
        }
        break;

    case 7:	/* Boston */
        if (CityScore > 500)
        {
            z = -100;
        }
        break;

    case 8:	/* Rio de Janeiro */
        if (CityScore > 500)
        {
            z = -100;
        }
        break;
    }

    ClearMes();
    //SendMes(z);

    if (z == -200)
    {
        DoLoseGame();
    }
}


void CheckGrowth()
{
    if (CityTime % 4 != 0)
    {
        return;
    }

    int currentPopulation = ((ResPop)+(ComPop * 8) + (IndPop * 8)) * 20;
    MessageEnumerator growthMessageId = MessageEnumerator::None;

    if (LastCityPop)
    {
        if ((LastCityPop < 2000) && (currentPopulation >= 2000))
        {
            growthMessageId = MessageEnumerator::ReachedTown;
        }
        if ((LastCityPop < 10000) && (currentPopulation >= 10000))
        {
            growthMessageId = MessageEnumerator::ReachedCity;
        }
        if ((LastCityPop < 50000L) && (currentPopulation >= 50000L))
        {
            growthMessageId = MessageEnumerator::ReachedCapital;
        }
        if ((LastCityPop < 100000L) && (currentPopulation >= 100000L))
        {
            growthMessageId = MessageEnumerator::ReachedMetropolis;
        }
        if ((LastCityPop < 500000L) && (currentPopulation >= 500000L))
        {
            growthMessageId = MessageEnumerator::ReachedMegalopolis;
        }
    }
    if (growthMessageId != MessageEnumerator::None &&
        growthMessageId != LastCategory)
    {
        SendMes(growthMessageId);
        LastCategory = growthMessageId;
    }

    LastCityPop = currentPopulation;

}



void SendMessages()
{
    if ((ScenarioID) && (ScoreType) && (ScoreWait))
    {
        ScoreWait--;
        if (!ScoreWait)
        {
            DoScenarioScore(ScoreType);
        }
    }

    CheckGrowth();

    TotalZPop = ResZPop + ComZPop + IndZPop;
    int PowerPop = NuclearPop + CoalPop;

    switch (CityTime % 64)
    {

    case 1:
        if ((TotalZPop / 4) >= ResZPop) /* need Res */
        {
            SendMes(MessageEnumerator::ResidentialNeeded);
        }
        break;

    case 5:
        if ((TotalZPop / 8) >= ComZPop) /* need Com */
        {
            SendMes(MessageEnumerator::CommercialNeeded);
        }
        break;

    case 10:
        if ((TotalZPop / 8) >= IndZPop) /* need Ind */
        {
            SendMes(MessageEnumerator::IndustrialNeeded);
        }
        break;

    case 14:
        if ((TotalZPop > 10) && ((TotalZPop << 1) > RoadTotal))
        {
            SendMes(MessageEnumerator::RoadsNeeded);
        }
        break;

    case 18:
        if ((TotalZPop > 50) && (TotalZPop > RailTotal))
        {
            SendMes(MessageEnumerator::RailNeeded);
        }
        break;

    case 22:
        if ((TotalZPop > 10) && (PowerPop == 0)) /* need Power */
        {
            SendMes(MessageEnumerator::PowerNeeded);
        }
        break;

    case 26:
        if ((ResPop > 500) && (StadiumPop == 0)) /* need Stad */
        {
            SendMes(MessageEnumerator::StadiumNeeded);
            ResCap = 1;
        }
        else
        {
            ResCap = 0;
        }
        break;

    case 28:
        if ((IndPop > 70) && (PortPop == 0))
        {
            SendMes(MessageEnumerator::SeaportNeeded);
            IndCap = 1;
        }
        else IndCap = 0;
        break;

    case 30:
        if ((ComPop > 100) && (APortPop == 0))
        {
            SendMes(MessageEnumerator::AirportNeeded);
            ComCap = 1;
        }
        else ComCap = 0;
        break;

    case 32: /* dec score for unpowered zones */
    {
        float TM = static_cast<float>(unPwrdZCnt + PwrdZCnt);

        /**
         * \fixme   This should use fuzzy comparisons here due to
         *          floating point drift.
         */
        if (TM)
        {
            if ((PwrdZCnt / TM) < .7)
            {
                SendMes(MessageEnumerator::BlackoutsReported);
            }
        }
    }
        break;

    case 35:
        if (PolluteAverage > /* 80 */ 60)
        {
            SendMes(MessageEnumerator::PollutionHigh);
        }
        break;

    case 42:
        if (CrimeAverage > 100)
        {
            SendMes(MessageEnumerator::CrimeHigh);
        }
        break;

    case 45:
        if ((TotalPop > 60) && (FireStPop == 0))
        {
            SendMes(MessageEnumerator::FireDepartmentNeeded);
        }
        break;

    case 48:
        if ((TotalPop > 60) && (PolicePop == 0))
        {
            SendMes(MessageEnumerator::PoliceDepartmentNeeded);
        }
        break;

    case 51:
        if (CityTax > 12)
        {
            SendMes(MessageEnumerator::TaxesHigh);
        }
        break;

    case 54:
        if ((RoadEffect < 20) && (RoadTotal > 30))
        {
            SendMes(MessageEnumerator::RoadsDeteriorating);
        }
        break;

    case 57:
        if ((FireEffect < 700) && (TotalPop > 20))
        {
            SendMes(MessageEnumerator::FireDefunded);
        }
        break;

    case 60:
        if ((PoliceEffect < 700) && (TotalPop > 20))
        {
            SendMes(MessageEnumerator::PoliceDefunded);
        }
        break;

    case 63:
        if (TrafficAverage > 60)
        {
            SendMes(MessageEnumerator::TrafficJamsReported);
        }
        break;
    }
}


void doMessage()
{
    bool firstTime = false;
    
    if (MessageId() == MessageEnumerator::None)
    {
        return;
    }
    
    if (MessageId() != MessageEnumerator::None)
    {
        LastMesTime = TickCount();
    }
    else if ((TickCount() - LastMesTime) > 1800)
    {
        MessageId(MessageEnumerator::None);
        return;
    }

    if (firstTime)
    {
        switch (MessageId())
        {
        case MessageEnumerator::TrafficJamsReported:
            if (RandomRange(0, 5) == 1)
            {
                MakeSound("city", "HonkHonk-Med");
            }
            else if (RandomRange(0, 5) == 1)
            {
                MakeSound("city", "HonkHonk-Low");
            }
            else if (RandomRange(0, 5) == 1)
            {
                MakeSound("city", "HonkHonk-High");
            }
            break;

        case MessageEnumerator::CrimeHigh:
        case MessageEnumerator::FireReported:
        case MessageEnumerator::TornadoReported:
        case MessageEnumerator::EarthquakeReported:
        case MessageEnumerator::PlaneCrashed:
        case MessageEnumerator::ShipWrecked:
        case MessageEnumerator::TrainCrashed:
        case MessageEnumerator::HelicopterCrashed:
            MakeSound("city", "Siren");
            break;

        case MessageEnumerator::MonsterReported:
            MakeSound("city", "Monster -speed [MonsterSpeed]");
            break;

        case MessageEnumerator::FirebombingReported:
            MakeSound("city", "Explosion-Low");
            MakeSound("city", "Siren");
            break;

        case  MessageEnumerator::NuclearMeltdownReported:
            MakeSound("city", "Explosion-High");
            MakeSound("city", "Explosion-Low");
            MakeSound("city", "Siren");
            break;

        case  MessageEnumerator::RiotsReported:
            MakeSound("city", "Siren");
            break;
        }

        firstTime = false;
    }

    if (MessageId() != MessageEnumerator::None)
    {
        if (MessageLocation() != Point<int>{0, 0})
        {
            // TODO: draw goto button
        }

        if (AutoGotoMessageLocation() && (MessageLocation() != Point<int>{0, 0}))
        {
            DoAutoGoto(MessageLocation().x, MessageLocation().y, GetIndString(301, MessageId()));
            MessageLocation({ 0, 0 });
        }
        else
        {
            SetMessageField(GetIndString(301, MessageId()));
        }
    }
    else
    {
        /*
        // picture message
        int pictId = -(MesNum);

        DoShowPicture(pictId);

        MessagePort = pictId; // resend text message

        if (AutoGo && (MesX || MesY))
        {
            DoAutoGoto(MesX, MesY, GetIndString(301, pictId));
            MesX = 0;
            MesY = 0;
        }
        */
    }
}
