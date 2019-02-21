#ifndef TB_DIAGNOSTICS_JSON_BUILDER
#define TB_DIAGNOSTICS_JSON_BUILDER

#include "const.h"
#include "log.h"
#include "json_builder_base.h"
#define ARDUINOJSON_USE_LONG_LONG 1
#include "ArduinoJson.h"

class TbDiagnosticsJsonBuilder : public JsonBuilderBase<Log::Entry, TB_DIAGNOSTICS_JSON_DOC_SIZE>
{
public:
	RetResult add(const Log::Entry *entry);
	static bool is_telemetry_code(Log::Code log_code);
};

#endif