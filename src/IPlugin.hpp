#pragma once
#include "./ArgsBuilder/Builder.hpp"
#include "IModel.hpp"
namespace d3156
{
    namespace PluginCore
    {
        class Core;

        class IPlugin
        {
        public:
            virtual ~IPlugin() {}

            virtual void registerModels(ModelsStorage &storage) = 0;

            /// \brief registerArgs Зарегистрировать аргументы командной строки
            /// \param bldr Анализатор командной строки
            /// \note Значения аргументов распарсятся до postInit
            virtual void registerArgs(Args::Builder &bldr) {}

            /// \brief postInit Дополнительная инициализация плагина, если требуется
            /// \note Вызывается после postInit всех моделей
            virtual void postInit() {}
        };
        // C ABI точки входа (имена должны совпасть с dlsym/GetProcAddress)
        extern "C" {
        IPlugin *create_plugin();
        void destroy_plugin(IPlugin *);
        }
    }
}