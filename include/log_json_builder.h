#ifndef LOG_JSON_H
#define LOG_JSON_H

#include "log.h"
#include "const.h"
#include "json_builder_base.h"

class LogJsonBuilder : public JsonBuilderBase<Log::Entry, LOG_JSON_DOC_SIZE>
{
public:
    RetResult add(const Log::Entry *entry);
};

#endif