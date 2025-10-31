#include "modulator_producer.h"
namespace OrangeSodium {
ModulationProducer::ModulationProducer(Context* context, ObjectID id) : m_context(context), id(id) {
    sample_rate = context->sample_rate;
    object_type = EObjectType::kModulatorProducer;
}

ModulationProducer::~ModulationProducer() {
    if (modulation_buffer) {
        delete modulation_buffer;
    }
    if (output_buffer) {
        delete output_buffer;
    }
}

}