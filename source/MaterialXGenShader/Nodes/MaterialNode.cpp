//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include <MaterialXGenShader/Nodes/MaterialNode.h>
#include <MaterialXGenShader/ShaderGenerator.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/GenContext.h>

MATERIALX_NAMESPACE_BEGIN

ShaderNodeImplPtr MaterialNode::create()
{
    return std::make_shared<MaterialNode>();
}

void MaterialNode::addClassification(ShaderNode& node) const
{
    const ShaderInput* surfaceshaderInput = node.getInput(ShaderNode::SURFACESHADER);
    const ShaderPort* connectedPort = surfaceshaderInput ? surfaceshaderInput->getConnection() : nullptr;

    if (connectedPort && connectedPort->getNode()->hasClassification(ShaderNode::Classification::SURFACE))
    {
        // This is a material node with a surfaceshader connected.
        // Add the classification from this shader.
        node.addClassification(connectedPort->getNode()->getClassification());
    }
}

void MaterialNode::emitFunctionCall(const ShaderNode& _node, GenContext& context, ShaderStage& stage) const
{
    DEFINE_SHADER_STAGE(stage, Stage::PIXEL)
    {
        ShaderNode& node = const_cast<ShaderNode&>(_node);
        ShaderInput* surfaceshaderInput = node.getInput(ShaderNode::SURFACESHADER);

        // Make sure we have a connection to a surfaceshader node upstream
        // NOTE: We also check if the connection is to the graph interface, 
        // as we don't support routing the surface shader through a graph interface.
        // It must be connected directly to the terminal material node.
        const ShaderPort* connectedPort = surfaceshaderInput ? surfaceshaderInput->getConnection() : nullptr;
        if (!connectedPort || 
            !connectedPort->getNode()->hasClassification(ShaderNode::Classification::SURFACE) ||
            (connectedPort->getNode() == node.getParent()))
        {
            // Just declare the output variable with default value.
            emitOutputVariables(node, context, stage);
            return;
        }

        const ShaderGenerator& shadergen = context.getShaderGenerator();
        const Syntax& syntax = shadergen.getSyntax();

        // Emit the function call for upstream surface shader.
        const ShaderNode* surfaceshaderNode = connectedPort->getNode();
        shadergen.emitFunctionCall(*surfaceshaderNode, context, stage);

        // Assing this result to the material output variable.
        const ShaderOutput* output = node.getOutput();
        shadergen.emitLine(syntax.getTypeName(output->getType()) + " " + output->getVariable() + " = " + surfaceshaderInput->getConnection()->getVariable(), stage);
    }
}

MATERIALX_NAMESPACE_END
