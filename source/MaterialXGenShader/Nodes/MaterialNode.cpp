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
    // Classify as a shader by default since a material is just a shader terminal.
    node.addClassification(ShaderNode::Classification::SHADER);

    // Find connected surfaceshader node.
    const ShaderInput* surfaceshaderInput = node.getInput(ShaderNode::SURFACESHADER);
    const ShaderNode* surfaceshaderNode = (surfaceshaderInput && surfaceshaderInput->getConnection()) ? surfaceshaderInput->getConnection()->getNode() : nullptr;

    // Make sure it's a sibling node and not the graph interface.
    if (surfaceshaderNode && (surfaceshaderNode->getParent() == node.getParent()))
    {
        // A connected surfaceshader was found so add its classification.
        node.addClassification(surfaceshaderNode->getClassification());
    }
}

void MaterialNode::emitFunctionCall(const ShaderNode& _node, GenContext& context, ShaderStage& stage) const
{
    DEFINE_SHADER_STAGE(stage, Stage::PIXEL)
    {
        ShaderNode& node = const_cast<ShaderNode&>(_node);
        const ShaderInput* surfaceshaderInput = node.getInput(ShaderNode::SURFACESHADER);
        const ShaderNode* surfaceshaderNode = (surfaceshaderInput && surfaceshaderInput->getConnection()) ? surfaceshaderInput->getConnection()->getNode() : nullptr;

        // Make sure it's a sibling node and not the graph interface.
        if (!(surfaceshaderNode && (surfaceshaderNode->getParent() == node.getParent())))
        {
            // This is a material node without a sibling surfaceshader connected.
            // Just declare the output variable with default value.
            emitOutputVariables(node, context, stage);
            return;
        }

        const ShaderGenerator& shadergen = context.getShaderGenerator();
        const Syntax& syntax = shadergen.getSyntax();

        // Emit the function call for upstream surface shader.
        shadergen.emitFunctionCall(*surfaceshaderNode, context, stage);

        // Assing this result to the material output variable.
        const ShaderOutput* output = node.getOutput();
        shadergen.emitLine(syntax.getTypeName(output->getType()) + " " + output->getVariable() + " = " + surfaceshaderInput->getConnection()->getVariable(), stage);
    }
}

MATERIALX_NAMESPACE_END
