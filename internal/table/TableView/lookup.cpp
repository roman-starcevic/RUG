// Copyright 2026 Roman Starcevic
// SPDX-License-Identifier: Apache-2.0

#include "TableView.hpp"

// ACTION
t_action	TableView::action( t_id_state state, t_id_symbol terminal ) const
{
	const t_action_span&	span = ((const t_action_span*)(m_base_ + m_hdr_->action_spans_offset))[state];
	const t_action_entry *entries = (const t_action_entry*)(m_base_ + m_hdr_->action_entries_offset);
	size_t					left;
	size_t					right;

	left = (size_t)span.offset;
	right = left + (size_t)span.count;

	// Binary lower_bound over exceptions.
	while (left < right)
	{
		size_t mid = left + (right - left) / 2;
		const t_action_entry& entry = entries[mid];

		if ((t_id_symbol)entry.terminal < terminal)
			left = mid + 1;
		else
			right = mid;
	}

	// Exception found
	if (left < (size_t)span.offset + (size_t)span.count)
	{
		const t_action_entry& entry = entries[left];
		
		if ((t_id_symbol)entry.terminal == terminal)
			return (unpack_packed_action(entry.action));
	}

	// No exception:
	// use the state's default action
	return (unpack_packed_action(span.default_action));
}

// GOTO
t_id_state	TableView::goto_state( t_id_state state, t_id_symbol non_terminal ) const
{
	const t_blob_span& span = ((const t_blob_span*)(m_base_ + m_hdr_->goto_spans_offset))[state];
	const t_goto_entry *entries = (const t_goto_entry*)(m_base_ + m_hdr_->goto_entries_offset);

	size_t	nt_index;
	size_t	left;
	size_t	right;

	if (non_terminal < CFG_FIRST_NON_TERMINAL)
		throw std::out_of_range("TableView::goto_state: symbol is not a non terminal");

	nt_index = (size_t)(non_terminal - CFG_FIRST_NON_TERMINAL);
	if (nt_index >= (size_t)m_hdr_->non_terminal_count)
		throw std::out_of_range("TableView::goto_state: invalid non terminal");

	left = (size_t)span.offset;
	right = left + (size_t)span.count;

	while (left < right)
	{
		size_t mid = left + (right - left) / 2;
		const t_goto_entry& entry = entries[mid];

		if ((size_t)entry.non_terminal < nt_index)
			left = mid + 1;
		else
			right = mid;
	}

	if (left >= (size_t)span.offset + (size_t)span.count)
		return (PARSING_INVALID_GOTO);

	const t_goto_entry& entry = entries[left];

	if ((size_t)entry.non_terminal!= nt_index)
		return (PARSING_INVALID_GOTO);

	return ((t_id_state)entry.target);
}

// Rule
const t_rule_entry&	TableView::rule( size_t rule_id ) const
{	return (((const t_rule_entry*)(m_base_ + m_hdr_->rule_entries_offset))[rule_id]);	}