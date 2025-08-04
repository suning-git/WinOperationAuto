#!/usr/bin/env python3
"""
LLM Handler for WinOpAuto
Supports OpenAI and DeepSeek APIs with configurable settings.
"""

import os
import json
import requests
from typing import Optional, Dict, Any

class LLMHandler:
    """Handles LLM interactions with multiple providers."""
    
    def __init__(self, secrets_file: str = "SECRET", prompts_file: str = "LLM_prompts.json"):
        """Initialize LLM handler with configuration from two files."""
        self.secrets = self._load_json_file(secrets_file)
        self.config = self._load_json_file(prompts_file)
        self.provider = self.config.get("provider", "openai").lower()
        
        print(f"[LLM] Initialized with provider: {self.provider}")
        print(f"[LLM] Using model: {self._get_model_name()}")
    
    def _load_json_file(self, filename: str) -> Dict[str, Any]:
        """Load configuration from JSON file."""
        script_dir = os.path.dirname(os.path.abspath(__file__))
        file_path = os.path.join(script_dir, filename)
        
        if not os.path.exists(file_path):
            raise FileNotFoundError(f"Configuration file not found: {file_path}")
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
            print(f"[CONFIG] Loaded {filename}")
            return data
        except json.JSONDecodeError as e:
            raise ValueError(f"Invalid JSON in {filename}: {e}")
        except Exception as e:
            raise Exception(f"Failed to load {filename}: {e}")
    
    def _get_model_name(self) -> str:
        """Get the appropriate model name for the current provider."""
        if self.provider == "openai":
            return self.config.get("openai_model", "gpt-3.5-turbo")
        elif self.provider == "deepseek":
            return self.config.get("deepseek_model", "deepseek-chat")
        return "unknown"
    
    def generate_response(self, input_sequence: str) -> Optional[str]:
        """Generate response using configured LLM provider."""
        if self.provider == "openai":
            return self._openai_request(input_sequence)
        elif self.provider == "deepseek":
            return self._deepseek_request(input_sequence)
        else:
            raise ValueError(f"Unsupported LLM provider: {self.provider}")
    
    def _openai_request(self, input_sequence: str) -> Optional[str]:
        """Make request to OpenAI API."""
        api_key = self.secrets.get("openai_api_key")
        if not api_key:
            raise ValueError("OpenAI API key not found in SECRET file")
        
        model = self.config.get("openai_model", "gpt-3.5-turbo")
        url = "https://api.openai.com/v1/chat/completions"
        
        headers = {
            "Authorization": f"Bearer {api_key}",
            "Content-Type": "application/json"
        }
        
        prompt = self._build_prompt(input_sequence)
        
        data = {
            "model": model,
            "messages": [
                {"role": "system", "content": "You are a helpful assistant that provides concise text completions and suggestions."},
                {"role": "user", "content": prompt}
            ],
            "max_tokens": self.config.get("max_tokens", 100),
            "temperature": self.config.get("temperature", 0.7)
        }
        
        try:
            print(f"[OPENAI] Making request to {model}...")
            response = requests.post(url, headers=headers, json=data, timeout=30)
            response.raise_for_status()
            
            result = response.json()
            content = result["choices"][0]["message"]["content"]
            
            print(f"[OPENAI] Response received: {len(content)} characters")
            return content
            
        except requests.exceptions.RequestException as e:
            print(f"[ERROR] OpenAI API request failed: {e}")
            return None
        except (KeyError, IndexError) as e:
            print(f"[ERROR] Invalid OpenAI API response format: {e}")
            return None
    
    def _deepseek_request(self, input_sequence: str) -> Optional[str]:
        """Make request to DeepSeek API."""
        api_key = self.secrets.get("deepseek_api_key")
        if not api_key:
            raise ValueError("DeepSeek API key not found in SECRET file")
        
        model = self.config.get("deepseek_model", "deepseek-chat")
        url = "https://api.deepseek.com/v1/chat/completions"
        
        headers = {
            "Authorization": f"Bearer {api_key}",
            "Content-Type": "application/json"
        }
        
        prompt = self._build_prompt(input_sequence)
        
        data = {
            "model": model,
            "messages": [
                {"role": "system", "content": "You are a helpful assistant that provides concise text completions and suggestions."},
                {"role": "user", "content": prompt}
            ],
            "max_tokens": self.config.get("max_tokens", 100),
            "temperature": self.config.get("temperature", 0.7)
        }
        
        try:
            print(f"[DEEPSEEK] Making request to {model}...")
            response = requests.post(url, headers=headers, json=data, timeout=30)
            response.raise_for_status()
            
            result = response.json()
            content = result["choices"][0]["message"]["content"]
            
            print(f"[DEEPSEEK] Response received: {len(content)} characters")
            return content
            
        except requests.exceptions.RequestException as e:
            print(f"[ERROR] DeepSeek API request failed: {e}")
            return None
        except (KeyError, IndexError) as e:
            print(f"[ERROR] Invalid DeepSeek API response format: {e}")
            return None
    
    def _build_prompt(self, input_sequence: str) -> str:
        """Build prompt for LLM based on input sequence."""
        prompt_template = self.config.get("prompt_template", 
            "Based on the user's input: '{input}'\n\nProvide a helpful completion or suggestion (keep it concise):")
        
        return prompt_template.format(input=input_sequence)


# Test function for development
def test_llm_handler():
    """Test function to verify LLM handler works."""
    try:
        llm = LLMHandler()
        test_input = "Hello, how are you"
        response = llm.generate_response(test_input)
        print(f"Test response: {response}")
    except Exception as e:
        print(f"Test failed: {e}")


if __name__ == "__main__":
    test_llm_handler()