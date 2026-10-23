// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "ABNFValidator.hpp"

bool	ABNFValidator::validate( void )
{
	if (m_ir_.rule_count() == 0)
	{
		t_source_span span;
		span.offset = 0;
		span.len = 0;
		m_diag_.error(span, "grammar contains no rules");
		return (false);
	}

	std::map<std::string, size_t>	rules;

	validate_rules(rules);
	validate_references(rules);
	validate_supported_nodes();

	return (!m_diag_.has_errors());
}

bool	ABNFValidator::validate_rules( std::map<std::string, size_t>& rules )
{
	std::string								name;
	const t_abnf_rule*						rule;
	std::map<std::string, size_t>::iterator	it;

	for (size_t i = 0; i < m_ir_.rule_count(); i++)
	{
		rule = &m_ir_.rule(i);
		name = canonical_name(rule->name);

		it = rules.find(name);

		if (!rule->incremental)
		{
			if (it != rules.end())
			{
				m_diag_.error(rule->name, "duplicate ABNF rule definition");
				continue ;
			}

			rules[name] = i;
		}
		else
		{
			if (it == rules.end())
			{
				m_diag_.error(rule->name, "incremental ABNF rule has no base definition");
				continue ;
			}
		}
	}

	return (!m_diag_.has_errors());
}

bool	ABNFValidator::validate_supported_nodes( void )
{
	bool	valid = true;

	for (size_t i = 0; i < m_ir_.rule_count(); i++)
	{
		if (!is_node_supported(m_ir_.rule(i).root_id))
			valid = false;
	}

	return (valid);
}

bool	ABNFValidator::validate_node_references( t_abnf_node_id id, const std::map<std::string, size_t>& rules )
{
	const t_abnf_node&	node = m_ir_.node(id);
	std::string			name;
	bool				valid;

	valid = true;

	if (node.type == ABNF_REF)
	{
		name = canonical_name(node.span);
		if (rules.find(name) == rules.end())
		{
			m_diag_.error(node.span, "reference to undefined ABNF rule");
			return (false);
		}
		return (true);
	}

	if (node.type == ABNF_REPEAT)
		return (validate_node_references(node.repeat.child, rules));

	if (node.type == ABNF_OPTION)
		return (validate_node_references(node.unary.child, rules));

	if (node.type == ABNF_ALT || node.type == ABNF_CONCAT)
	{
		for (size_t i = 0; i < node.child.count; i++)
		{
			if (!validate_node_references(m_ir_.child(node, i),rules))
				valid = false;
		}
		return (valid);
	}

	// CHAR_VAL / NUM_VAL / PROSE_VAL
	// leafs :no rule to check
	return (true);
}

bool	ABNFValidator::validate_references( const std::map<std::string, size_t>& rules )
{
	for (size_t i = 0; i < m_ir_.rule_count(); i++)
		validate_node_references(m_ir_.rule(i).root_id, rules);

	return (!m_diag_.has_errors());
}


