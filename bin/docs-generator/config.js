function parseIni (iniText) {
  const sections = {}
  let currentSection = null
  let lastComment = ''
  let defaultValue = ''

  iniText.split(/\r?\n/).forEach((line) => {
    const trimmedLine = line.trim()
    if (trimmedLine.startsWith(';') && !/;\s+\S+\s+=\s+.*/.test(trimmedLine)) {
      if (trimmedLine.includes('default value:')) {
        defaultValue = trimmedLine.split('default value:')[1].trim()
      } else {
        lastComment += ' ' + trimmedLine.slice(1).trim()
      }
    } else if (trimmedLine.startsWith('[') && trimmedLine.endsWith(']')) {
      currentSection = trimmedLine.slice(1, -1)
      sections[currentSection] = []
      lastComment = ''
    } else if (currentSection) {
      const keyValue = trimmedLine.replace('; ', '').split('=')
      if (keyValue.length === 2) {
        sections[currentSection].push({
          key: keyValue[0].trim(),
          value: keyValue[1].trim(),
          defaultValue,
          description: lastComment
        })
        lastComment = ''
        defaultValue = ''
      }
    }
  })

  return sections
}

function createConfigMd (sections) {
  let md = '<!-- This file is generated by bin/docs-generator/config.js -->\n'
  md += '<!-- Do not edit this file directly. -->\n\n'
  md += '# Configuration basics\n'
  md += `
The configuration file is a simple INI \`socket.ini\` file in the root of the project.
The file is read on startup and the values are used to configure the project.
Sometimes it's useful to overide the values in \`socket.ini\` or keep some of the values local (e.g. \`[ios] simulator_device\`)
or secret (e.g. \`[ios] codesign_identity\`, \`[ios] provisioning_profile\`, etc.)
This can be done by creating a file called \`.ssrc\` in the root of the project.
It is possible to override both Command Line Interface (CLI) and Configuration File (INI) options.

Example:

\`socket.ini\`:
\`\`\`ini
; other settings

[build]

headless = false

; other settings
\`\`\`

\`.ssrc\`:
\`\`\`ini
[build]

platform = ios ; override the \`ssc build --platform\` CLI option


[settings.ios] ; override the \`[ios]\` section in \`socket.ini\`

codesign_identity = "iPhone Developer: John Doe (XXXXXXXXXX)"
distribution_method = "ad-hoc"
provisioning_profile = "johndoe.mobileprovision"
simulator_device = "iPhone 15"
\`\`\`

<tonic-toaster-inline
  title="Note"
  type="info">
    Note that "~" alias won't expand to the home directory in any of the configuration files.
    Use the full path instead.
</tonic-toaster-inline>
`
  md += '\n'
  Object.entries(sections).forEach(([sectionName, settings]) => {
    md += `# \`${sectionName}\`\n`
    md += '\n'
    md += 'Key | Default Value | Description\n'
    md += ':--- | :--- | :---\n'
    settings.forEach(({ key, defaultValue, description }) => {
      md += `${key} | ${defaultValue} | ${description}\n`
    })
    md += '\n'
  })
  return md
}

// Generate config.md
export function generateConfig (source) {
  const startMarker = 'constexpr auto gDefaultConfig = R"INI('
  const endMarker = ')INI";'

  const startIndex = source.indexOf(startMarker)

  const remainingData = source.slice(startIndex + startMarker.length)
  const endIndex = remainingData.indexOf(endMarker)

  if (startIndex === -1 || endIndex === -1) {
    console.error('Start or end marker not found')
  }

  const extractedText = remainingData.slice(0, endIndex)

  const sections = parseIni(extractedText)
  return createConfigMd(sections)
}
