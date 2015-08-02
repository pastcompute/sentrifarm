# Sentrifarm : *Should I reap or should I wait?*

Sentrifarm will provide a simple and effective management tool to Australia farmers
* sentrifarm informs the farmer withat a glance whether it is safe to harvest any crop
* sentrifarm provides realtime and historical data on weather, crop and astronomical conditions
* sentrifarm is self sufficient for power (solar) and communications (not reliant on the GSM network)
* sentrifarm is designed to be modular and expandable
* sentrifarm is designed to be interfaced with other systems
* sentrifarm allows the farmer to investigate, experiment and discover new information about their enterprise

# Background
Farming in Australia presents many unique challenges.

## Climate challenges

Not least are our summer climactic conditions: high temperatures, strong winds and dry soil and vegetation.
Together these provide the ingredients for a disaster such as the [2015 Samson Flat](https://en.wikipedia.org/wiki/2015_Sampson_Flat_bushfires)
fires that burned nearly 50000 acres and destroyed houses and businesses and farmland in country
not that far northeast of Adelaide.
Broadacre farming presents particular hazards:
* grain harvest is often due over the summer December - January
period,
* many past fires have been started by 'headers' -
the large [combine harvesters](https://en.wikipedia.org/wiki/Combine_harvester#Combine_heads)
used to reap and collect the ripened grain
* a crop fire started by a header will move incredibly fast,
destroying unharvested crop and able to morph into a full scale bushfire if farmland borders scrubby hills or forest.

## Record keeping challenges

The modern farmer is more and more reliant on agricultural science and data analysis, with the family farm having to be run like a business. It is no longer sufficient to rely on rule of thumb, simple experience or heirloom methods to remain competitive and effective as operating costs increase, labour becomes scarcer, drought more common and conditions more variable.

## Problem: *Should I reap or should I wait?* - confirming localised weather conditions prior to reaping

To remain safe a farmer must rely on more than official weather forecasts:
* Australian farms can have _large_ paddocks. 200 acres may be 'small'
* Dry soil or vegetation amplifies ambient temperature effects
* The [Grass Fire Danger Index](http://www.csiro.au/en/Research/Environment/Extreme-Events/Bushfire/Fire-danger-meters/Grass-fire-danger-meter) is a more accurate predictor of dangerous conditions 
* Variable and windy conditions means the weather can vary from one side of a paddock to the other!
* The published official weather forecasts are insufficient
* The farmer must directly check conditions on each and every paddock before deploying their header
* This usually means driving out, which consumes time, and fuel. Paddocks in Australia are often geographically disparate as well as large.

## Problem: Recording and analysis of multiple sensor inputs

Being able to monitor and record measurements from a wide variety of sensors over a wide geographical area presents a great opportunity to further enhance farming methods, by finding better answers to questions such as:
* what crop should I plant in that paddock this time?
* should I rest that paddock?
* when should I sow?
* when should I apply fertiliser?
* when should I reap?
* how has the performance of a particular crop trended under particular conditions over the last 5 years?
* what $$$ return will I get if I plant these crops over this area?

To be sustainable the sensor network is constrained:
* sensors need to be self sufficient for power : there is no mains power in a paddock.
* sensors need to be independent of commercial telecommunication networks
** even if the cost were relatively cheap ($10/pa for a sim card?) the management of 10's or 100's doesnt scale
* sensors need to be robust : farms are hard on equipment, and paddocks exposed to all kinds of eather
* sensors need to be maintainable : using cheap parts and open software protocols allows farmers to make on the run repairs
* sensors need to be accessible: on the ground information via an NFC-tap would be incredibly useful

# Solution : sentrifarm

Sentrifarm provides a distributed network of sensor nodes that can be deployed in a manner suited customised to meet the specific topographic nature of the farm.

Determing whether local conditions are safe will vary according to local requirements.  Sentrifarm can collect any combination of ambient and ground temperature, wind speed and direction, air pressure, ground moisture and average crop colour to suit, reporting this information from multiple locations to a central server located on the farm. Data can be updated at regular intervals, even more frequently than hourly if required.

Intelligent software can then analyse the collected sensor data and provide the farmer with a simple Go / No Go or Green / Red traffic light for reaping, allowing them to decide, *Should I reap or should I wait?*

Along with the warning sensors, additional information including rainfall, dew, frost and UV light can also be recorded. This data can be recorded for a long period of time allowing for in-depth analysis of trends.

Further, by recording previously overlooked sensors and data at regular intervals including across the day, this provides for the possibility of finding hitherto-unknown correlations between different sensors and new opportunities for innovation in farm management.

Other applications, such as distributed pollen recording and smoke detection, may also become possible, subject to development of proper analysis methods.

# Business Case

Sentrifarm addresses a gap in the market, through both cheaper hardware costs and open design.

Many existing farm systems are:
* very expensive for what you get, or
* use proprietary protocols and lock up data away from the farmer, or

Farmers are very busy people.  Some may have the skill, motivation or expertise to maintain a sensor deployment, and the open source nature of the system provides that opportunity for them.  For the vast majority, they would rather outsouce the installation and ongoing service, providing a local business opportunity on the ground that is harder to be overtaken or locked up by a multinational (case in point: proprietary CAN bus systems in tractors)
