#include "serialize.h"
#include <model/graphsmodel.h>
#include <zeno/utils/logger.h>
#include <model/modeldata.h>
#include <model/modelrole.h>


static void serializeGraph(SubGraphModel* pModel, GraphsModel* pGraphsModel, QStringList const &graphNames, QJsonArray& ret, QString const &graphIdPrefix)
{
	const QString& name = pModel->name();
	const NODES_DATA& nodes = pModel->nodes();

	for (const NODE_DATA& node : nodes)
	{
		QString ident = node[ROLE_OBJID].toString();
        ident = graphIdPrefix + ident;
		QString name = node[ROLE_OBJNAME].toString();
		const INPUT_SOCKETS& inputs = node[ROLE_INPUTS].value<INPUT_SOCKETS>();
        const OUTPUT_SOCKETS& outputs = node[ROLE_OUTPUTS].value<OUTPUT_SOCKETS>();
		PARAMS_INFO params = node[ROLE_PARAMETERS].value<PARAMS_INFO>();
		int opts = node[ROLE_OPTIONS].toInt();
		/*QStringList options;
		if (opts & OPT_ONCE)
		{
			options.push_back("ONCE");
		}
		if (opts & OPT_MUTE)
		{
			options.push_back("MUTE");
		}
		if (opts & OPT_PREP)
		{
			options.push_back("PREP");
		}
		if (opts & OPT_VIEW)
		{
			options.push_back("VIEW");
		}*/
        QString noOnceIdent;
        if (opts & OPT_ONCE) {
            noOnceIdent = ident;
            ident = ident + ".RUNONCE";
        }

        if (opts & OPT_MUTE) {
            ret.push_back(QJsonArray({ "addNode", "HelperMute", ident }));
        } else {

            if (graphNames.indexOf(name) != -1)
            {
                auto nextGraphIdPrefix = ident + "/";
                SubGraphModel* pSubModel = pGraphsModel->subGraph(name);
                serializeGraph(pSubModel, pGraphsModel, graphNames, ret, nextGraphIdPrefix);
                //ret.push_back(QJsonArray({ "pushSubgraph", ident, name }));
                //serializeGraph(pSubModel, pGraphsModel, graphNames, ret);
                //ret.push_back(QJsonArray({ "popSubgraph", ident, name }));

            } else {
                ret.push_back(QJsonArray({ "addNode", name, ident }));
            }
        }

        auto outputIt = outputs.begin();

        for (INPUT_SOCKET input : inputs)
        {
            auto inputName = input.info.name;

            if (opts & OPT_MUTE) {
                if (outputIt != outputs.end()) {
                    OUTPUT_SOCKET output = *outputIt++;
                    inputName = output.info.name; // HelperMute forward all inputs to outputs by socket name
                } else {
                    inputName += ".DUMMYDEP";
                }
            }

            if (input.linkIndice.isEmpty())
            {
                const QVariant& defl = input.info.defaultValue;
                if (!defl.isNull())
                {
                    QVariant::Type varType = defl.type();
                    if (varType == QVariant::Double)
                    {
                        ret.push_back(QJsonArray({ "setNodeInput", ident, inputName, defl.toDouble() }));
                    }
                    else if (varType == QVariant::Int)
                    {
                        ret.push_back(QJsonArray({ "setNodeInput", ident, inputName, defl.toInt() }));
                    }
                    else if (varType == QVariant::String)
                    {
                        ret.push_back(QJsonArray({ "setNodeInput", ident, inputName, defl.toString() }));
                    }
                    else if (varType == QVariant::Bool)
                    {
                        ret.push_back(QJsonArray({ "setNodeInput", ident, inputName, defl.toBool() }));
                    }
                    else if (varType != QVariant::Invalid)
                    {
                        zeno::log_warn("bad qt variant type {}", defl.typeName() ? defl.typeName() : "(null)");
                        Q_ASSERT(false);
                    }
                }
            }
            else
            {
                for (QPersistentModelIndex linkIdx : input.linkIndice)
                {
                    Q_ASSERT(linkIdx.isValid());
                    const QString& outSock = linkIdx.data(ROLE_OUTSOCK).toString();
                    const QString& outId = linkIdx.data(ROLE_OUTNODE).toString();
                    ret.push_back(QJsonArray({ "bindNodeInput", ident, inputName, outId, outSock }));
                }
            }
        }

		for (PARAM_INFO param_info : params)
		{
			QVariant value = param_info.value;
			QVariant::Type varType = value.type();

			if (param_info.name == "_KEYS")
			{
				int j;
				j = 0;
			}

			if (varType == QVariant::Double)
			{
				ret.push_back(QJsonArray({"setNodeParam", ident, param_info.name, value.toDouble()}));
			}
			else if (varType == QVariant::Int)
			{
				ret.push_back(QJsonArray({"setNodeParam", ident, param_info.name, value.toInt()}));
			}
			else if (varType == QVariant::String)
			{
				ret.push_back(QJsonArray({"setNodeParam", ident, param_info.name, value.toString()}));
			}
			else if (varType == QVariant::Bool)
			{
				ret.push_back(QJsonArray({ "setNodeParam", ident, param_info.name, value.toBool() }));
			}
			else if (varType != QVariant::Invalid)
			{
                zeno::log_warn("bad qt variant type {}", value.typeName() ? value.typeName() : "(null)");
				Q_ASSERT(false);
			}
		}

        if (opts & OPT_ONCE) {
            ret.push_back(QJsonArray({ "addNode", "HelperOnce", noOnceIdent }));
            for (OUTPUT_SOCKET output : outputs) {
                if (output.info.name == "DST") continue;
                ret.push_back(QJsonArray({"bindNodeInput", noOnceIdent, output.info.name, ident, output.info.name}));
            }
            ret.push_back(QJsonArray({"completeNode", ident}));
            ident = noOnceIdent;//must before OPT_VIEW branch
        }

		ret.push_back(QJsonArray({"completeNode", ident}));

		if (opts & OPT_VIEW) {
            for (OUTPUT_SOCKET output : outputs)
            {
                //if (output.info.name == "DST") continue;//qmap wants to put DST/SRC as first socket, skip it
                auto viewerIdent = ident + ".TOVIEW";
                ret.push_back(QJsonArray({"addNode", "ToView", viewerIdent}));
                ret.push_back(QJsonArray({"bindNodeInput", viewerIdent, "object", ident, output.info.name}));
                ret.push_back(QJsonArray({"completeNode", viewerIdent}));
                break;
            }
        }

		/*if (opts & OPT_MUTE) {
            auto inputIt = inputs.begin();
            for (OUTPUT_SOCKET output : outputs)
            {
                if (inputIt == inputs.end()) break;
                INPUT_SOCKET input = *++inputIt;
                input.info.name
            }
        }*/

        // mock options at editor side, done
		/*for (QString optionName : options)
		{
			ret.push_back(QJsonArray({"setNodeOption", ident, optionName}));
		}*/
	}
}

void serializeScene(GraphsModel* pModel, QJsonArray& ret)
{
	//QJsonArray item = { "clearAllState" };
    //ret.push_back(item);

	QStringList graphs;
	for (int i = 0; i < pModel->rowCount(); i++)
		graphs.push_back(pModel->subGraph(i)->name());

    SubGraphModel* pSubModel = pModel->subGraph("main");
    serializeGraph(pSubModel, pModel, graphs, ret, "");
}
