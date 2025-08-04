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


def get_context_info(input_sequence: str) -> str:
    """Generate context information about the input for better LLM understanding."""
    
    context_hints = []
    
    # Detect patterns
    if "\n" in input_sequence:
        context_hints.append("multi-line text")
    
    if input_sequence.strip().endswith("?"):
        context_hints.append("question")
    
    if input_sequence.strip().startswith(("Dear", "Hi", "Hello")):
        context_hints.append("email/message")
    
    if any(char.isdigit() for char in input_sequence):
        if any(op in input_sequence for op in ["+", "-", "*", "/", "="]):
            context_hints.append("calculation/math")
    
    if any(keyword in input_sequence.lower() for keyword in ["def ", "function", "class ", "import", "#include"]):
        context_hints.append("code")
    
    return ", ".join(context_hints) if context_hints else "general text"