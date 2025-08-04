#!/usr/bin/env python3
"""
Input cleaning utilities for better LLM processing.
"""

def clean_input_for_llm(input_sequence: str) -> str:
    """Clean up the input sequence minimally while preserving context for LLM."""
    
    # Only do minimal cleaning - preserve most special keys for context
    cleaned = input_sequence
    
    # Only remove truly redundant patterns
    noise_patterns = [
        "[SHIFT]",  # Shift effect is already in capitalization
        "[CTRL]",   # Ctrl combinations are handled separately 
        "[ALT]",    # Alt combinations are handled separately
    ]
    
    for pattern in noise_patterns:
        cleaned = cleaned.replace(pattern, "")
    
    # Clean up multiple spaces but preserve intentional spacing
    while "  " in cleaned:
        cleaned = cleaned.replace("  ", " ")
    
    # Don't strip - preserve leading/trailing context
    
    # Always return the cleaned version, even if short
    return cleaned
