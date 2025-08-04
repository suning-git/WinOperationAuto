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
    
    def __init__(self, secrets_file: str = "SECRET", config_file: str = "LLM_config.json", debug: bool = False):
        """Initialize LLM handler with configuration from two files."""
        self.debug = debug
        self.secrets = self._load_json_file(secrets_file)
        self.config = self._load_json_file(config_file)
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
    
    def _load_markdown_file(self, filename: str) -> str:
        """Load content from Markdown file."""
        script_dir = os.path.dirname(os.path.abspath(__file__))
        file_path = os.path.join(script_dir, filename)
        
        if not os.path.exists(file_path):
            raise FileNotFoundError(f"Markdown file not found: {file_path}")
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            print(f"[CONFIG] Loaded {filename}")
            return content
        except Exception as e:
            raise Exception(f"Failed to load {filename}: {e}")
    
    def _get_system_prompt(self) -> str:
        """Get system prompt from file."""
        system_prompt_file = self.config.get("system_prompt_file", "system_prompt.md")
        content = self._load_markdown_file(system_prompt_file)
        
        # Remove markdown headers and convert to plain text
        lines = content.split('\n')
        plain_lines = []
        for line in lines:
            # Skip markdown headers (lines starting with #)
            if line.strip().startswith('#'):
                continue
            # Skip empty lines at the beginning
            if not plain_lines and not line.strip():
                continue
            plain_lines.append(line)
        
        # Join lines and clean up
        return '\n'.join(plain_lines).strip()
    

    
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
        
        system_prompt = self._get_system_prompt()
        user_prompt = self._build_user_prompt(input_sequence)
        
        # Reasoning models (o3, o1 series) use different parameter names
        reasoning_models = ["o3", "o3-mini", "o1", "o1-mini", "o1-preview"]
        is_reasoning_model = any(reasoning_model in model.lower() for reasoning_model in reasoning_models)
        
        data = {
            "model": model,
            "messages": [
                {"role": "system", "content": system_prompt},
                {"role": "user", "content": user_prompt}
            ]
        }
        
        # Use appropriate token parameter based on model type
        if is_reasoning_model:
            # Reasoning models need much more tokens (they use tokens for internal reasoning)
            reasoning_tokens = self.config.get("max_tokens", 100)
            # For reasoning models, we need extra tokens beyond the configured amount
            # because they use tokens for reasoning AND for the final output
            total_tokens = max(reasoning_tokens * 10, 1000)  # At least 1000 tokens for reasoning models
            data["max_completion_tokens"] = total_tokens
            if self.debug:
                print(f"[DEBUG] Reasoning model: using {total_tokens} max_completion_tokens (reasoning needs extra tokens)")
        else:
            data["max_tokens"] = self.config.get("max_tokens", 100)
        
        # Some reasoning models don't support temperature parameter
        if not is_reasoning_model:
            data["temperature"] = self.config.get("temperature", 0.7)
        
        try:
            if is_reasoning_model:
                print(f"[OPENAI] Making request to reasoning model {model} (using max_completion_tokens)...")
            else:
                print(f"[OPENAI] Making request to {model}...")
            
            # Debug: Print the request data
            if self.debug:
                print(f"[DEBUG] Request data: {data}")
            
            response = requests.post(url, headers=headers, json=data, timeout=30)
            
            # Debug: Print response status
            if self.debug:
                print(f"[DEBUG] HTTP Status Code: {response.status_code}")
                print(f"[DEBUG] Response Headers: {dict(response.headers)}")
            
            # Better error handling for different HTTP status codes
            if response.status_code == 404:
                print(f"[ERROR] Model '{model}' not found. This could mean:")
                print(f"[ERROR] 1. The model name is incorrect")
                print(f"[ERROR] 2. The model is not available to your API key")
                print(f"[ERROR] 3. The model requires special access or billing tier")
                return None
            elif response.status_code == 401:
                print(f"[ERROR] Authentication failed. Check your API key.")
                return None
            elif response.status_code == 429:
                print(f"[ERROR] Rate limit exceeded. Please try again later.")
                return None
            
            response.raise_for_status()
            
            result = response.json()
            
            # Debug: Print the full response
            if self.debug:
                print(f"[DEBUG] Full API response: {result}")
            
            # Debug: Check response structure
            if "choices" not in result:
                if self.debug:
                    print(f"[DEBUG] No 'choices' in response!")
                return None
            
            if len(result["choices"]) == 0:
                if self.debug:
                    print(f"[DEBUG] Empty choices array!")
                return None
            
            choice = result["choices"][0]
            if self.debug:
                print(f"[DEBUG] First choice: {choice}")
            
            if "message" not in choice:
                if self.debug:
                    print(f"[DEBUG] No 'message' in choice!")
                return None
            
            message = choice["message"]
            if self.debug:
                print(f"[DEBUG] Message object: {message}")
            
            content = message.get("content", "")
            if self.debug:
                print(f"[DEBUG] Content: '{content}' (length: {len(content)})")
            
            print(f"[OPENAI] Response received: {len(content)} characters")
            return content
            
        except requests.exceptions.RequestException as e:
            print(f"[ERROR] OpenAI API request failed: {e}")
            if hasattr(e, 'response') and e.response is not None:
                try:
                    error_details = e.response.json()
                    if 'error' in error_details:
                        print(f"[ERROR] API Error Details: {error_details['error']}")
                except:
                    pass
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
        
        system_prompt = self._get_system_prompt()
        user_prompt = self._build_user_prompt(input_sequence)
        
        data = {
            "model": model,
            "messages": [
                {"role": "system", "content": system_prompt},
                {"role": "user", "content": user_prompt}
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
    
    def _build_user_prompt(self, input_sequence: str) -> str:
        """Build user prompt for LLM based on input sequence."""
        user_prompt_template = self.config.get("user_prompt_template", 
            "The user's input sequence: '{input}'\n\nNext keyboard input:")
        
        return user_prompt_template.format(input=input_sequence)
    
    def _build_prompt(self, input_sequence: str) -> str:
        """Build prompt for LLM based on input sequence (deprecated, use _build_user_prompt)."""
        return self._build_user_prompt(input_sequence)


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