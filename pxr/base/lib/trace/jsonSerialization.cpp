//
// Copyright 2018 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/base/trace/jsonSerialization.h"

#include "pxr/pxr.h"
#include "pxr/base/js/json.h"
#include "pxr/base/js/utils.h"

#include "pxr/base/trace/eventData.h"
#include "pxr/base/trace/singleEventTreeReport.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
/// JS utility functions
////////////////////////////////////////////////////////////////////////////////

template<typename T>
static
typename std::enable_if< 
    !std::is_same<T, JsObject>::value && 
    !std::is_same<T, JsArray>::value && 
    !std::is_same<T, std::string>::value, boost::optional<T> >::type
_JsGet(const boost::optional<JsValue>& js)
{
    if (js && js->Is<T>()) {
        return js->Get<T>();
    }
    return boost::none;
}

template<typename T>
static
typename std::enable_if<
    std::is_same<T, JsObject>::value ||
    std::is_same<T, JsArray>::value ||
    std::is_same<T, std::string>::value, const T* >::type
_JsGet(const boost::optional<JsValue>& js)
{
    if (js && js->Is<T>()) {
        return &js->Get<T>();
    }
    return nullptr;
}

template <typename T,
    typename ReturnType = 
        typename std::conditional<
            std::is_same<T, JsObject>::value || 
            std::is_same<T, JsArray>::value || 
            std::is_same<T, std::string>::value,
            const T*, boost::optional<T> >::type>
ReturnType _JsGetValue(const JsObject& js, const std::string& key) {
    return _JsGet<T>(JsFindValue(js, key));
}

// Chrome stores timestamps in microseconds whild Trace stores them in ticks.
static TraceEvent::TimeStamp
_MicrosecondsToTicks(double us)
{
    return static_cast<TraceEvent::TimeStamp>(
        us*1000.0 / ArchGetNanosecondsPerTick());
}

static double
_TicksToMicroSeconds(TraceEvent::TimeStamp t)
{
    return ArchTicksToNanoseconds(t)/1000.0;
}

// TraceEvent::EventType is stored as a string in JSON.
static const char*
_EventTypeToString(TraceEvent::EventType t) {
    switch(t) {
        case TraceEvent::EventType::Begin: return "Begin";
        case TraceEvent::EventType::End: return "End";
        case TraceEvent::EventType::Counter: return "Counter";
        case TraceEvent::EventType::Timespan: return "Timespan";
        case TraceEvent::EventType::ScopeData: return "Data";
        case TraceEvent::EventType::Unknown: return "Unknown";
    }
    return "Unknown";
}

static TraceEvent::EventType
_EventTypeFromString(const std::string& s) {
    if (s == "Begin") {
        return TraceEvent::EventType::Begin;
    } else if (s == "End") {
        return TraceEvent::EventType::End;
    } else if (s == "Counter") {
        return TraceEvent::EventType::Counter;
    } else if (s == "Timespan") {
        return TraceEvent::EventType::Timespan;
    } else if (s == "Data") {
        return TraceEvent::EventType::ScopeData;
    }
    return TraceEvent::EventType::Unknown;
}

// Helper struct to hold data needed to reconstruct an event list.
// Since events are read from json out of order, they are placed in
// unorderedEvents first. Later they are sorted and added to the eventList.
struct EventListConstructionData {
    TraceEventList eventList;
    std::vector<TraceEvent> unorderedEvents;
};

using ChromeThreadId = std::string;
using ChromeConstructionMap = 
    std::map<ChromeThreadId, EventListConstructionData>;

// Returns a JSON representatoin of a Trace event. This format is a "raw" format
// that does not match the Chrome format.
JsValue
_TraceEventToJSON(const TfToken& key, const TraceEvent& e)
{
    JsObject event;
    event["key"] = JsValue(key.GetString());
    event["category"] = JsValue(static_cast<uint64_t>(e.GetCategory()));
    event["type"] = JsValue(_EventTypeToString(e.GetType()));
    switch (e.GetType()) {
        case TraceEvent::EventType::Begin:
        case TraceEvent::EventType::End:
            event["ts"] = JsValue(_TicksToMicroSeconds(e.GetTimeStamp()));
            break;
        case TraceEvent::EventType::Counter:
            event["ts"] = JsValue(_TicksToMicroSeconds(e.GetTimeStamp()));
            event["value"] = JsValue(e.GetCounterValue());
            break;
        case TraceEvent::EventType::ScopeData:
            event["ts"] = JsValue(_TicksToMicroSeconds(e.GetTimeStamp()));
            event["data"] = e.GetData().ToJson();
            break;
        case TraceEvent::EventType::Timespan:
            event["start"] = 
                JsValue(_TicksToMicroSeconds(e.GetStartTimeStamp()));
            event["end"] = JsValue(_TicksToMicroSeconds(e.GetEndTimeStamp()));
            break;
        case TraceEvent::EventType::Unknown:
            break;
    }
    return event;
}

// Reads a "raw" format JSON object and adds it to the eventListData if it can.
void
_TraceEventFromJSON(
    const JsValue& jsValue,
    EventListConstructionData& eventListData)
{
    if (!jsValue.IsObject()) { return; }

    TraceEventList& list = eventListData.eventList;
    std::vector<TraceEvent>&  unorderedEvents = eventListData.unorderedEvents;

    const JsObject& js = jsValue.GetJsObject();
    const std::string* keyStr = _JsGetValue<std::string>(js, "key");
    boost::optional<uint64_t> category = _JsGetValue<uint64_t>(js, "category");
    const std::string* typeStr = _JsGetValue<std::string>(js, "type");
    boost::optional<double> tsMicroSeconds = 
        _JsGetValue<double>(js, "ts");
    boost::optional<TraceEvent::TimeStamp> ts;
    if (tsMicroSeconds) {
        ts = _MicrosecondsToTicks(*tsMicroSeconds);
    }
    if (keyStr && category && typeStr) {
        TraceEvent::EventType type = _EventTypeFromString(*typeStr);
        switch (type) {
            case TraceEvent::EventType::Unknown:
                break;
            case TraceEvent::EventType::Begin:
                if (ts) {
                    unorderedEvents.emplace_back(
                        TraceEvent::Begin,
                        list.CacheKey(*keyStr),
                        *ts,
                        *category);
                }
                break;
            case TraceEvent::EventType::End:
                if (ts) {
                    unorderedEvents.emplace_back(
                        TraceEvent::End,
                        list.CacheKey(*keyStr),
                        *ts,
                        *category);
                }
                break;
            case TraceEvent::EventType::Timespan:
                {
                    boost::optional<TraceEvent::TimeStamp> start = 
                        _JsGetValue<TraceEvent::TimeStamp>(js, "start");
                    boost::optional<TraceEvent::TimeStamp> end = 
                        _JsGetValue<TraceEvent::TimeStamp>(js, "end");
                    if (start && end) {
                        unorderedEvents.emplace_back(
                            TraceEvent::Timespan,
                            list.CacheKey(*keyStr),
                            *start,
                            *end,
                            *category);
                    }
                }
                break;
            case TraceEvent::EventType::Counter:
                {
                    boost::optional<double> value = 
                        _JsGetValue<double>(js, "value");
                    if (ts && value) {
                        TraceEvent event(TraceEvent::Counter,
                            list.CacheKey(*keyStr), 
                            *value,
                            *category);
                        event.SetTimeStamp(*ts);
                        unorderedEvents.emplace_back(event);
                    }
                }
                break;
            case TraceEvent::EventType::ScopeData:
                if (ts) {
                    if (boost::optional<JsValue> dataValue =
                        JsFindValue(js, "data")) {
                        if (dataValue->Is<bool>()) {
                            TraceEvent event(
                                TraceEvent::Data,
                                list.CacheKey(*keyStr), 
                                dataValue->Get<bool>(),
                                *category);
                            event.SetTimeStamp(*ts);
                            unorderedEvents.emplace_back(event);
                        } else if (dataValue->Is<double>()) {
                            TraceEvent event(
                                TraceEvent::Data,
                                list.CacheKey(*keyStr), 
                                dataValue->Get<double>(),
                                *category);
                            event.SetTimeStamp(*ts);
                            unorderedEvents.emplace_back(event);
                        } else if (dataValue->Is<uint64_t>()) {
                            TraceEvent event(
                                TraceEvent::Data,
                                list.CacheKey(*keyStr),
                                dataValue->Get<uint64_t>(),
                                *category);
                            event.SetTimeStamp(*ts);
                            unorderedEvents.emplace_back(event);
                        } else if (dataValue->Is<int64_t>()) {
                            TraceEvent event(
                                TraceEvent::Data,
                                list.CacheKey(*keyStr),
                                dataValue->Get<int64_t>(),
                                *category);
                            event.SetTimeStamp(*ts);
                            unorderedEvents.emplace_back(event);
                        } else if (dataValue->Is<std::string>()) {
                            TraceEvent event(
                                TraceEvent::Data,
                                list.CacheKey(*keyStr),
                                list.StoreData(dataValue->GetString().c_str()),
                                *category);
                            event.SetTimeStamp(*ts);
                            unorderedEvents.emplace_back(event);
                        }
                    }
                }
                break;
        }
    }
}

namespace {

// This class created a JSON array that a JSON objects per thread in the
// collection which has Counter events and Data events. This data is need in 
// addition to the Chrome Format JSON to fully reconstruct a TraceCollection.
class _CollectionEventsToJson : public TraceCollection::Visitor {
public:
    const JsArray& GetThreads() const { return _threads;}


    virtual void OnBeginCollection() override {}
    virtual void OnEndCollection() override {}

    
    virtual void OnBeginThread(const TraceThreadId& threadId) override {
        _events.clear();
    }

    virtual void OnEndThread(const TraceThreadId& threadId) override {
        if (!_events.empty()) {
            JsObject thread;
            thread["thread"] = JsValue(threadId.ToString());
            thread["events"] = _events;
            _events.clear();
            _threads.emplace_back(std::move(thread));
        }
    }

    virtual bool AcceptsCategory(TraceCategoryId categoryId) override {
        return true;
    }

    virtual void OnEvent(
        const TraceThreadId& threadId, 
        const TfToken& key, 
        const TraceEvent& event) override {

        // Only convert Counter and Data events. The other types will be in the
        // chrome format.
        switch (event.GetType()) {
            case TraceEvent::EventType::ScopeData:
            case TraceEvent::EventType::Counter:
                _events.emplace_back(_TraceEventToJSON(key, event));
                break;
            case TraceEvent::EventType::Begin:
            case TraceEvent::EventType::End:
            case TraceEvent::EventType::Timespan:
            case TraceEvent::EventType::Unknown:
                break;
        }
    }
private:
    JsArray _threads;
    JsArray _events;
};

}

JsValue 
Trace_JSONSerialization::CollectionToJSON(const TraceCollection& collection)
{
    JsObject libtraceData;
    JsArray extraTraceEvents;
    // Convert Counter and Data events to JSON.
    {
        _CollectionEventsToJson eventsToJson;
        collection.Iterate(eventsToJson);
        libtraceData["threadEvents"] = eventsToJson.GetThreads();
    }

    TraceSingleEventTreeReport report;
    collection.Iterate(report);
    JsObject traceObj = report.GetGraph()->CreateChromeTraceObject();

    // Add the extra lib trace data to the Chrome trace object.
    traceObj["libTraceData"] = libtraceData;
    return traceObj;
}

// This function converts Chrome trace events into TraceEvents and adds them to 
// output.
static 
void
_ImportChromeEvents(
    const JsArray& traceEvents, ChromeConstructionMap& output)
{
    for (const JsValue& event : traceEvents) {
        if (const JsObject* eventObj = _JsGet<JsObject>(event)) {
            const ChromeThreadId* tid = 
                _JsGetValue<ChromeThreadId>(*eventObj, "tid");
            boost::optional<double> ts = _JsGetValue<double>(*eventObj, "ts");
            const std::string* name = 
                _JsGetValue<std::string>(*eventObj, "name");
            const std::string* ph = _JsGetValue<std::string>(*eventObj, "ph");
            boost::optional<uint64_t> catId = 
                _JsGetValue<uint64_t>(*eventObj, "libTraceCatId");

            if (tid && ts && name && ph) {
                TraceKey key = output[*tid].eventList.CacheKey(*name);
                if (!catId) {
                    catId = 0;
                }
                if (*ph == "B") {
                    output[*tid].unorderedEvents.emplace_back(
                        TraceEvent::Begin,
                        key,
                        _MicrosecondsToTicks(*ts),
                        *catId);
                } else if (*ph == "E") {
                    output[*tid].unorderedEvents.emplace_back(
                        TraceEvent::End,
                        key,
                        _MicrosecondsToTicks(*ts),
                        *catId);
                } else if (*ph == "X") {
                    boost::optional<double> dur = 
                        _JsGetValue<double>(*eventObj, "dur");
                    if (dur) {
                        output[*tid].unorderedEvents.emplace_back(
                            TraceEvent::Timespan, key, 
                            _MicrosecondsToTicks(*ts),
                            _MicrosecondsToTicks(*ts+*dur), *catId);
                    }
                }
            }
        }
    }
}

// Creates a TraceEventList from EventListConstructionData.
static std::unique_ptr<TraceEventList>
_ConstructEventList(EventListConstructionData& data)
{
    TF_AXIOM(data.eventList.IsEmpty());
    // TraceEventLists are sorted by timestamp.
    std::sort(data.unorderedEvents.begin(), data.unorderedEvents.end(), 
    [] (const TraceEvent& lhs, const TraceEvent& rhs) -> bool {
        TraceEvent::TimeStamp l_time = lhs.GetTimeStamp();
        TraceEvent::TimeStamp r_time = rhs.GetTimeStamp();
        return l_time < r_time;
    });

    // Add the events to the eventList.
    // TODO: make a constructor that takes an event vector so we don't have to 
    // make copies?
    for (const TraceEvent& e : data.unorderedEvents) {
        data.eventList.EmplaceBack(e);
    }
    data.unorderedEvents.clear();
    return std::unique_ptr<TraceEventList>(
        new TraceEventList(std::move(data.eventList)));
}

std::unique_ptr<TraceCollection> 
Trace_JSONSerialization::CollectionFromJSON(const JsValue& jsValue) {
    const JsObject* traceObj = _JsGet<JsObject>(jsValue);
    const JsArray* chromeEvents = 0;
    if (traceObj) {
        chromeEvents = _JsGetValue<JsArray>(*traceObj, "traceEvents");
    } else {
        chromeEvents = _JsGet<JsArray>(jsValue);
    }
    const JsObject* traceDataObj =
        traceObj ? _JsGetValue<JsObject>(*traceObj, "libTraceData") : nullptr;

    ChromeConstructionMap constMap;
    // Add events from the chrome trace format.
    if (chromeEvents) {
        _ImportChromeEvents(*chromeEvents, constMap);
    }
    // Add events from the libTrace specific json.
    if (traceDataObj) {
        if (const JsArray* threadEvents = 
            _JsGetValue<JsArray>(*traceDataObj, "threadEvents")) {
            for (const JsValue& v : *threadEvents) {
                if (const JsObject* threadObj = _JsGet<JsObject>(v)) {
                    const ChromeThreadId* threadId = 
                        _JsGetValue<ChromeThreadId>(*threadObj, "thread");
                    const JsArray* eventArray = 
                            _JsGetValue<JsArray>(*threadObj, "events");
                    if (threadId && eventArray) {
                        for (const JsValue& eventValue : *eventArray) {
                            _TraceEventFromJSON(
                                eventValue,
                                constMap[*threadId]);
                        }
                    }
                }
            }
        }
    }

    // Create the event lists and collection.
    if (!constMap.empty()) {
        std::unique_ptr<TraceCollection> collection(new TraceCollection());
        for (ChromeConstructionMap::value_type& c : constMap) {
            collection->AddToCollection(
                    TraceThreadId(c.first),
                    _ConstructEventList(c.second));
        }
        return collection;
    }
    return nullptr;
}


PXR_NAMESPACE_CLOSE_SCOPE