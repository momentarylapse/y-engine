#include <lib/kapi/KabaExporter.h>
#include <plugins/PluginManager.h>

KABA_PACKAGE_EXPORT_BEGIN

KABA_PACKAGE_EXPORT void export_symbols(kaba::Exporter* e) {
	PluginManager::export_kaba_package_y(e);
}
KABA_PACKAGE_EXPORT_END


