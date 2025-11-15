#!/usr/bin/env python3
"""
Extract HTML from webui_html.c and update preview files in webui_preview/
"""

import re
import os

def extract_html_from_c_function(c_file_path, function_name):
    """Extract HTML content from a C function that returns a string literal."""
    with open(c_file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Find the function start
    func_start_pattern = rf'const char \*{function_name}\(void\)\s*\{{'
    func_start_match = re.search(func_start_pattern, content)
    
    if not func_start_match:
        print(f"Warning: Function {function_name} not found")
        return None
    
    # Start from after the opening brace
    start_pos = func_start_match.end()
    
    # Find the matching closing brace (handle nested braces)
    brace_count = 1
    pos = start_pos
    while pos < len(content) and brace_count > 0:
        if content[pos] == '{':
            brace_count += 1
        elif content[pos] == '}':
            brace_count -= 1
        pos += 1
    
    if brace_count != 0:
        print(f"Warning: Could not find matching closing brace for {function_name}")
        return None
    
    function_body = content[start_pos:pos-1]
    
    # Find the return statement - need to match until the final semicolon before the closing brace
    # Look for "return" followed by string literals until we find a semicolon at the end
    # The return statement ends with "; before the closing brace
    return_match = re.search(r'return\s+(.*);', function_body, re.DOTALL)
    if not return_match:
        print(f"Warning: No return statement found in {function_name}")
        return None
    
    return_statement = return_match.group(1).strip()
    
    # Extract all string literals (handles concatenated strings across multiple lines)
    # Pattern matches: "..." or "..." "..." etc.
    # This regex handles escaped quotes and newlines within strings
    string_pattern = r'"(?:[^"\\]|\\.)*"'
    strings = re.findall(string_pattern, return_statement, re.DOTALL)
    
    if not strings:
        print(f"Warning: No strings found in {function_name}")
        return None
    
    # Join all strings (they're already quoted, so we need to remove quotes from each)
    html_parts = []
    for s in strings:
        # Remove outer quotes
        unquoted = s[1:-1]
        # Decode escape sequences
        unquoted = unquoted.replace('\\"', '"')
        unquoted = unquoted.replace('\\n', '\n')
        unquoted = unquoted.replace('\\t', '\t')
        unquoted = unquoted.replace('\\\\', '\\')
        html_parts.append(unquoted)
    
    html = ''.join(html_parts)
    
    # Convert absolute paths to relative paths for preview files
    # Map server routes to preview HTML files
    path_mappings = {
        'href="/"': 'href="index.html"',
        'href="/vl53l1x"': 'href="status.html"',
        'href="/inputassembly"': 'href="inputassembly.html"',
        'href="/outputassembly"': 'href="outputassembly.html"',
        'href="/ota"': 'href="ota.html"',
        "href='/'" : "href='index.html'",
        "href='/vl53l1x'": "href='status.html'",
        "href='/inputassembly'": "href='inputassembly.html'",
        "href='/outputassembly'": "href='outputassembly.html'",
        "href='/ota'": "href='ota.html'",
    }
    
    for old_path, new_path in path_mappings.items():
        html = html.replace(old_path, new_path)
    
    return html

def main():
    c_file = 'components/webui/src/webui_html.c'
    
    if not os.path.exists(c_file):
        print(f"Error: {c_file} not found")
        return 1
    
    # Map function names to output files
    mappings = {
        'webui_get_index_html': 'webui_preview/index.html',
        'webui_get_status_html': 'webui_preview/status.html',
        'webui_get_ethernetip_html': 'webui_preview/outputassembly.html',
        'webui_get_input_assembly_html': 'webui_preview/inputassembly.html',
        'webui_get_ota_html': 'webui_preview/ota.html',
    }
    
    for func_name, output_file in mappings.items():
        print(f"Extracting {func_name} -> {output_file}...")
        html = extract_html_from_c_function(c_file, func_name)
        
        if html:
            # Add dummy data for preview files
            if 'inputassembly.html' in output_file:
                # Add dummy data script for Input Assembly - replace the catch block in updateStatus
                dummy_script = """
<script>
// Dummy data for preview
const dummyInputData = {
  input_assembly_100: {
    raw_bytes: [
      0xD4, 0x04, 0x00, 0x2A, 0x00, 0x1E, 0x00, 0x10, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00
    ]
  }
};

// Override updateStatus to use dummy data when API fails
(function() {
  const originalUpdateStatus = window.updateStatus;
  window.updateStatus = function() {
    fetch('/api/status')
      .then(r => {
        if (!r.ok) throw new Error('HTTP ' + r.status);
        return r.json();
      })
      .then(data => {
        if (data.input_assembly_100 && data.input_assembly_100.raw_bytes) {
          const container = document.getElementById('bytes_container');
          if (container) {
            container.innerHTML = '';
            for (let i = 0; i < data.input_assembly_100.raw_bytes.length; i++) {
              const byteValue = data.input_assembly_100.raw_bytes[i];
              const row = createByteRow(i, byteValue);
              container.appendChild(row);
            }
          }
        }
      })
      .catch(err => {
        // Use dummy data when API fails (preview mode)
        console.log('Using dummy data for preview');
        const container = document.getElementById('bytes_container');
        if (container && typeof createByteRow === 'function') {
          container.innerHTML = '';
          for (let i = 0; i < dummyInputData.input_assembly_100.raw_bytes.length; i++) {
            const byteValue = dummyInputData.input_assembly_100.raw_bytes[i];
            const row = createByteRow(i, byteValue);
            container.appendChild(row);
          }
        }
      });
  };
})();
</script>
"""
                # Insert before closing </body> tag
                html = html.replace('</body>', dummy_script + '</body>')
                
            elif 'outputassembly.html' in output_file:
                # Add dummy data script for Output Assembly - replace the catch block in updateStatus
                dummy_script = """
<script>
// Dummy data for preview
const dummyOutputData = {
  output_assembly_150: {
    raw_bytes: [
      0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
      0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    ]
  }
};

// Override updateStatus to use dummy data when API fails
(function() {
  const originalUpdateStatus = window.updateStatus;
  window.updateStatus = function() {
    fetch('/api/status')
      .then(r => {
        if (!r.ok) throw new Error('HTTP ' + r.status);
        return r.json();
      })
      .then(data => {
        if (data.output_assembly_150 && data.output_assembly_150.raw_bytes) {
          const container = document.getElementById('bytes_container');
          if (container) {
            container.innerHTML = '';
            for (let i = 0; i < data.output_assembly_150.raw_bytes.length; i++) {
              const byteValue = data.output_assembly_150.raw_bytes[i];
              const row = createByteRow(i, byteValue);
              container.appendChild(row);
            }
          }
        }
      })
      .catch(err => {
        // Use dummy data when API fails (preview mode)
        console.log('Using dummy data for preview');
        const container = document.getElementById('bytes_container');
        if (container && typeof createByteRow === 'function') {
          container.innerHTML = '';
          for (let i = 0; i < dummyOutputData.output_assembly_150.raw_bytes.length; i++) {
            const byteValue = dummyOutputData.output_assembly_150.raw_bytes[i];
            const row = createByteRow(i, byteValue);
            container.appendChild(row);
          }
        }
      });
  };
})();
</script>
"""
                # Insert before closing </body> tag
                html = html.replace('</body>', dummy_script + '</body>')
            
            # Ensure output directory exists
            os.makedirs(os.path.dirname(output_file), exist_ok=True)
            
            # Write HTML file
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(html)
            print(f"  [OK] Updated {output_file}")
        else:
            print(f"  [FAIL] Failed to extract HTML from {func_name}")
    
    print("\nPreview files updated successfully!")
    return 0

if __name__ == '__main__':
    exit(main())

