#!/usr/bin/env python3
"""
Input processing script for WinOpAuto
Reads input_events.txt, processes keyboard events, and generates key outputs.

Debug Mode:
Set environment variable DEBUG=1 to see detailed input/output messages.
Example: set DEBUG=1 && python process_input.py
"""

import json
import sys
import os
from typing import List, Dict, Optional

def read_input_events(filepath: str) -> List[Dict]:
    """Read and parse input events from JSON file."""
    events = []
    
    if not os.path.exists(filepath):
        print(f"[ERROR] Input file not found: {filepath}")
        return events
    
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                line = line.strip()
                if not line:
                    continue
                    
                try:
                    event = json.loads(line)
                    events.append(event)
                except json.JSONDecodeError as e:
                    print(f"[WARNING] Failed to parse line {line_num}: {e}")
                    continue
                    
    except Exception as e:
        print(f"[ERROR] Failed to read input file: {e}")
        
    print(f"[INFO] Read {len(events)} events from {filepath}")
    return events



def extract_input_sequence(events: List[Dict]) -> str:
    """Extract the sequence of user inputs (characters + special keys + mouse events)."""
    input_sequence = []
    
    for event in events:
        event_type = event.get("type")
        
        if event_type == "keyboard":
            # Only process keydown events
            if event.get("action") == "keydown":
                char = event.get("char")
                key = event.get("key", "")
                
                if char and len(char) == 1:  # Single character available
                    input_sequence.append(char)
                elif key:  # Use key name for special keys
                    # Convert some common key names to more readable format for LLM understanding
                    key_mapping = {
                        "SPACE": " ",
                        "ENTER": "\n",
                        "TAB": "\t",
                        "BACKSPACE": "[BACKSPACE]",
                        "DELETE": "[DELETE]",
                        "UP_ARROW": "[UP]",
                        "DOWN_ARROW": "[DOWN]",
                        "LEFT_ARROW": "[LEFT]",
                        "RIGHT_ARROW": "[RIGHT]",
                        "HOME": "[HOME]",
                        "END": "[END]",
                        "PAGE_UP": "[PAGEUP]",
                        "PAGE_DOWN": "[PAGEDOWN]",
                        "INSERT": "[INSERT]",
                        # Only filter out modifier keys that don't add context
                        "SHIFT": "",      # Shift effect is already reflected in capitalization
                        "CTRL": "",       # Ctrl combinations are handled separately
                        "ALT": "",        # Alt combinations are handled separately
                        "CAPS_LOCK": "[CAPS]"
                    }
                    
                    mapped_key = key_mapping.get(key, f"[{key}]")
                    if mapped_key:  # Only append if not empty
                        input_sequence.append(mapped_key)
                        
        elif event_type == "mouse":
            # Process mouse events for context
            action = event.get("action", "")
            x = event.get("x", 0)
            y = event.get("y", 0)
            
            # Map mouse actions to readable format
            mouse_mapping = {
                "leftdown": "MouseLeftClick",
                "leftup": "",  # Only show the click, not the release
                "rightdown": "MouseRightClick", 
                "rightup": "",
                "middledown": "MouseMiddleClick",
                "middleup": ""
            }
            
            mouse_event = mouse_mapping.get(action, "")
            if mouse_event:
                # Include position for context (rounded to nearest 50 pixels for privacy)
                rounded_x = (x // 50) * 50
                rounded_y = (y // 50) * 50
                input_sequence.append(f"[{mouse_event}({rounded_x},{rounded_y})]")
    
    sequence = ''.join(input_sequence)
    print(f"[INFO] Extracted input sequence: '{sequence}'")
    return sequence

def process_with_llm(input_sequence: str) -> Optional[str]:
    """Process input sequence with LLM and get response."""
    import os
    from llm_handler import LLMHandler
    from input_cleaner import clean_input_for_llm
    
    # Check if we're in debug mode (can be set via environment variable)
    debug_mode = False
    
    try:
        # Clean the input for better LLM processing
        cleaned_input = clean_input_for_llm(input_sequence)
        
        if debug_mode:
            print(f"[CLEAN] Input to LLM: {cleaned_input}")
        else:
            print(f"[CLEAN] Input processed: {len(cleaned_input)} characters")
        
        # Use cleaned input for LLM
        llm = LLMHandler(debug=debug_mode)
        response = llm.generate_response(cleaned_input)
        
        if response and response.strip():
            if debug_mode:
                print(f"[DEBUG] LLM response: '{response}'")
            else:
                print(f"[LLM] Generated completion: '{response}'")
            # Don't strip leading/trailing spaces - they're important for text completion
            return response
        else:
            print(f"[LLM] No completion generated")
            return None
            
    except Exception as e:
        print(f"[ERROR] LLM processing failed: {e}")
        return None

def write_output(output_keys: str, output_file: str):
    """Write the output keys to file for C++ to read."""
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(output_keys + '\n')
        print(f"[SUCCESS] Output written to {output_file}")
    except Exception as e:
        print(f"[ERROR] Failed to write output file: {e}")

def main():
    """Main processing function."""
    # Use current directory (where this script is located)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    input_file = os.path.join(script_dir, "input_events.txt")
    output_file = os.path.join(script_dir, "python_output.txt")
    
    print("[START] Processing input events...")
    
    # Read input events
    events = read_input_events(input_file)
    if not events:
        print("[ERROR] No events to process")
        return 1
    
    # Extract the input sequence for LLM (Ctrl events already filtered by input_cleaner.py)
    input_sequence = extract_input_sequence(events)
    
    # Process with LLM and generate output
    output_keys = process_with_llm(input_sequence)
    
    if output_keys:
        write_output(output_keys, output_file)
        print("[SUCCESS] Completion ready for injection")
        return 0
    else:
        # Write empty output to indicate no match
        write_output("", output_file)
        print("[INFO] No completion generated")
        return 0

if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code)