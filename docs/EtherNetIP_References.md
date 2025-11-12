# EtherNet/IP Reference Documentation

This document provides a reference guide to key ODVA (Open DeviceNet Vendors Association) EtherNet/IP publications relevant to the ESP32-P4 EtherNet/IP project.

## Overview

These publications are official ODVA documents that provide comprehensive information on various aspects of EtherNet/IP technology, from high-level overviews to detailed technical specifications and implementation guidelines.

## Core Documentation

### Technology Overview

**Pub 138 - Technology Overview Series: EtherNet/IP**
- **Purpose**: Comprehensive introduction to EtherNet/IP technology
- **Content**: Architecture, protocols, features, and applications in industrial automation
- **Relevance**: Essential reading for understanding EtherNet/IP fundamentals
- **Use Case**: Project planning, architecture decisions, stakeholder education
- **Direct Link**: [https://www.odva.org/wp-content/uploads/2024/04/PUB00138R8_Ethernet.pdf](https://www.odva.org/wp-content/uploads/2024/04/PUB00138R8_Ethernet.pdf)
- **Synopsis**: EtherNet/IP uses standard Ethernet/TCP/IP to transport CIP (Common Industrial Protocol). Supports both explicit messaging (request-response) and implicit messaging (real-time I/O). Uses object-oriented model with classes (Identity, Message Router, Connection Manager, etc.). Key advantage: leverages standard Ethernet infrastructure while providing deterministic industrial communication. Supports both TCP (explicit) and UDP (implicit) transport.

### Interoperability & Testing

**Pub 95 - EtherNet/IP Interoperability Test Procedures**
- **Purpose**: Standardized testing procedures to ensure device interoperability
- **Content**: Test methodologies, procedures, and requirements for EtherNet/IP devices
- **Relevance**: Critical for ensuring our ESP32-P4 implementation works with other EtherNet/IP devices
- **Use Case**: Pre-deployment testing, PlugFest participation, conformance validation
- **Direct Link**: [https://www.odva.org/wp-content/uploads/2020/05/PUB00095_PF-Test-Procedure-v9.pdf](https://www.odva.org/wp-content/uploads/2020/05/PUB00095_PF-Test-Procedure-v9.pdf)
- **Synopsis**: Defines test procedures for PlugFest events and conformance testing. Tests include: encapsulation header validation, connection establishment, explicit messaging, implicit I/O, device discovery, and error handling. Critical tests: RegisterSession, UnregisterSession, SendRRData, SendUnitData. Devices must pass these tests to be considered interoperable. Tests verify compliance with EtherNet/IP specification and ensure devices work with scanners/adapters from different vendors.

**Pub 81 - Performance Test Methodology for EtherNet/IP Devices**
- **Purpose**: Detailed methodology for conducting performance tests
- **Content**: Test procedures, measurement techniques, and evaluation criteria
- **Relevance**: Guides performance testing of our ESP32-P4 implementation
- **Use Case**: Performance validation, optimization, compliance verification
- **Direct Link**: [https://www.odva.org/wp-content/uploads/2020/05/PUB00081R0_Performance_Test_Methodology_for_EtherNetIP_Devices.pdf](https://www.odva.org/wp-content/uploads/2020/05/PUB00081R0_Performance_Test_Methodology_for_EtherNetIP_Devices.pdf)
- **Synopsis**: Defines how to measure device performance: connection establishment time, response time for explicit messages, I/O update rates, jitter, packet loss, and throughput. Test setup requirements: controlled network conditions, measurement tools, and test scenarios. Key metrics: RPI (Requested Packet Interval) compliance, end-to-end latency, and network utilization. Performance targets vary by device class and application requirements.

**Pub 80 - Performance Test Terminology for EtherNet/IP Devices**
- **Purpose**: Defines terminology and metrics for performance testing
- **Content**: Standardized terms and definitions for performance evaluation
- **Relevance**: Ensures consistent performance measurement and reporting
- **Use Case**: Performance benchmarking, documentation, test planning
- **Direct Link**: [https://www.odva.org/wp-content/uploads/2020/05/PUB00080R0_Performance_Test_Terminology_for_EtherNetIP_Devices.pdf](https://www.odva.org/wp-content/uploads/2020/05/PUB00080R0_Performance_Test_Terminology_for_EtherNetIP_Devices.pdf)
- **Synopsis**: Standardizes performance terminology: RPI (Requested Packet Interval), end-to-end delay, jitter, throughput, connection overhead, network utilization, and packet loss rate. Defines measurement points: application-to-application, network ingress/egress, and device processing time. Clarifies difference between connection establishment time and data transfer latency. Essential reference when documenting or comparing device performance metrics.

## Network Infrastructure & Installation

**Pub 148 - EtherNet/IP Media Planning & Installation Manual**
- **Purpose**: Guidelines for planning and installing EtherNet/IP physical media
- **Content**: Cable selection, connectors, grounding, network topology, installation best practices
- **Relevance**: Ensures proper network infrastructure for reliable operation
- **Use Case**: Network design, installation planning, troubleshooting
- **Note**: Available in English and Japanese
- **Direct Link**: [https://www.odva.org/wp-content/uploads/2020/05/PUB00148R0_ETHERNETIP_MEDIA_PLANNING_AND_INSTALLATION_MANUAL.pdf](https://www.odva.org/wp-content/uploads/2020/05/PUB00148R0_ETHERNETIP_MEDIA_PLANNING_AND_INSTALLATION_MANUAL.pdf)
- **Synopsis**: Use Category 5e or better twisted-pair cable for 100BASE-TX. Maximum segment length: 100m (328 ft). Proper grounding is critical: use shielded cable, ground at one point only, avoid ground loops. Use industrial-grade connectors (M12 or RJ45 with protective boots). Avoid running Ethernet cables parallel to power cables; cross at 90° if necessary. Use managed switches for industrial environments. Plan for proper cable management and strain relief.

**Pub 35 - EtherNet/IP Network Infrastructure Guide**
- **Purpose**: Comprehensive guide to EtherNet/IP network infrastructure
- **Content**: Network architecture, switch selection, VLAN configuration, QoS settings
- **Relevance**: Critical for designing robust EtherNet/IP networks
- **Use Case**: Network architecture design, infrastructure planning
- **Direct Link**: [https://www.odva.org/wp-content/uploads/2020/05/PUB00035R0_EtherNetIP_Network_Infrastructure_Guide.pdf](https://www.odva.org/wp-content/uploads/2020/05/PUB00035R0_EtherNetIP_Network_Infrastructure_Guide.pdf)
- **Synopsis**: Use managed switches with QoS support. Configure VLANs to segment control traffic from IT traffic. Set DSCP priorities: Urgent (55), Scheduled (47), High (43), Low (31), Explicit (27). Enable IGMP snooping for multicast efficiency. Use full-duplex links only; disable auto-negotiation in critical paths. Plan for network redundancy if uptime is critical. Switches should support port mirroring for diagnostics. Consider network segmentation to limit broadcast domains.

**Pub 88 - Recommended Operation for Switches Running Relay Agent and Option 82**
- **Purpose**: Guidelines for DHCP relay agent configuration in EtherNet/IP networks
- **Content**: DHCP relay agent setup, Option 82 configuration, best practices
- **Relevance**: Important for DHCP-based IP address assignment scenarios
- **Use Case**: Network configuration, DHCP deployment planning
- **Direct Link**: [https://www.odva.org/wp-content/uploads/2020/06/PUB00088R0_Recommended_Operation_for_Switches_Running_Relay_Agent_and_Option_82.pdf](https://www.odva.org/wp-content/uploads/2020/06/PUB00088R0_Recommended_Operation_for_Switches_Running_Relay_Agent_and_Option_82.pdf)
- **Synopsis**: DHCP relay agents forward DHCP requests across VLANs/subnets. Option 82 adds switch/port information to DHCP requests for better IP assignment control. Configure relay agent on switches between device VLANs and DHCP server. Option 82 helps identify device location (switch ID, port number) for centralized IP management. Use when devices are on different subnets than DHCP server. Not applicable for static IP configurations (our current implementation).

## Device Development & Implementation

**Pub 70 - Recommended Functionality for EtherNet/IP Devices**
- **Purpose**: Guidelines for recommended features and functionality in EtherNet/IP devices
- **Content**: Standard features, optional capabilities, implementation recommendations
- **Relevance**: Ensures our ESP32-P4 device meets industry expectations
- **Use Case**: Feature planning, development roadmap, compliance assessment
- **Direct Link**: [https://www.odva.org/wp-content/uploads/2020/05/PUB00070R0_Recommended_Functionality_for_EtherNetIP_Devices.pdf](https://www.odva.org/wp-content/uploads/2020/05/PUB00070R0_Recommended_Functionality_for_EtherNetIP_Devices.pdf)
- **Synopsis**: Essential objects: Identity (Class 0x01), Message Router (Class 0x02), Connection Manager (Class 0x06). Recommended: TCP/IP Interface (Class 0xF5), Ethernet Link (Class 0xF6), Assembly (Class 0x04). Should support: Get_Attribute_All service, device reset, connection establishment, both explicit and implicit messaging. Optional but valuable: QoS object, configuration persistence, device diagnostics. Defines minimum feature set for "good citizen" EtherNet/IP devices that work well in mixed-vendor networks.

**Pub 28 - Recommended IP Addressing Methods for EtherNet/IP™ Devices**
- **Purpose**: Best practices for IP address configuration in EtherNet/IP devices
- **Content**: Static vs. DHCP addressing, address conflict detection, configuration methods
- **Relevance**: Directly relevant to our static IP with ACD implementation
- **Use Case**: IP configuration design, ACD implementation, network integration
- **Direct Link**: [https://www.odva.org/technology-standards/document-library/](https://www.odva.org/technology-standards/document-library/) (Search for "Pub 28" or "IP Addressing Methods")
- **Synopsis**: **CRITICAL FOR OUR PROJECT**: Recommends Address Conflict Detection (ACD) for static IP assignment per RFC 5227. Devices should probe for conflicts before assigning static IP. If conflict detected, device must NOT use the address. Supports both DHCP and static IP; static IP preferred for deterministic behavior. Configuration methods: BOOTP, DHCP, manual configuration, or persistent storage (NVS). ACD should run before IP assignment and continue monitoring after assignment. This publication directly validates our RFC 5227 compliant ACD implementation approach.

**Pub 316 - Guidelines for Using Device Level Ring (DLR) with EtherNet/IP**
- **Purpose**: Implementation guidelines for Device Level Ring topology
- **Content**: DLR architecture, ring supervisor operation, redundancy configuration
- **Relevance**: Important for future DLR support (currently not implemented due to single-port limitation)
- **Use Case**: Future feature planning, topology considerations
- **Note**: ESP32-P4 has single Ethernet port, so DLR not applicable in current design
- **Direct Link**: [https://www.odva.org/wp-content/uploads/2020/06/PUB00316R0_Guidelines_for_Using_Device_Level_Ring_with_EtherNetIP.pdf](https://www.odva.org/wp-content/uploads/2020/06/PUB00316R0_Guidelines_for_Using_Device_Level_Ring_with_EtherNetIP.pdf)
- **Synopsis**: DLR provides ring topology with <3ms recovery time. Requires two Ethernet ports per device (not applicable to ESP32-P4). One device acts as Ring Supervisor, others are Ring Nodes. Ring Supervisor sends beacon frames; detects breaks when beacon doesn't return. On break detection, ring opens at supervisor, creating linear topology. Ring automatically heals when break is repaired. Requires DLR-capable switches or devices with dual ports. Provides high availability for critical control applications.

**Pub 213 - Quick Start for Vendors**
- **Purpose**: Quick start guide for vendors developing EtherNet/IP products
- **Content**: Essential concepts, development process, key resources, requirements
- **Relevance**: Valuable onboarding resource for new developers
- **Use Case**: Project kickoff, developer training, quick reference
- **Direct Link**: [https://www.odva.org/wp-content/uploads/2020/05/PUB00213R0_EtherNetIP_Developers_Guide.pdf](https://www.odva.org/wp-content/uploads/2020/05/PUB00213R0_EtherNetIP_Developers_Guide.pdf)
- **Synopsis**: Step-by-step guide for EtherNet/IP product development. Key steps: understand CIP object model, implement required objects (Identity, Message Router, Connection Manager), support encapsulation protocol, implement explicit and implicit messaging, test with ODVA tools, participate in PlugFest, obtain conformance certification. Lists essential resources: EtherNet/IP specification, CIP specification, development tools, test equipment. Provides timeline estimates and common pitfalls. Good starting point for new EtherNet/IP developers.

## Additional ODVA Core Publications

**PUB00123R1 - The Common Industrial Protocol (CIP™) and the Family of CIP Networks**
- **Purpose**: Comprehensive overview of CIP protocol and its network adaptations
- **Content**: CIP architecture, object model, services, messaging, device profiles, network adaptations (DeviceNet, ControlNet, EtherNet/IP, CompoNet)
- **Relevance**: Foundational understanding of CIP, which is the basis for EtherNet/IP
- **Use Case**: Understanding CIP fundamentals, protocol architecture, object-oriented design
- **Direct Link**: [https://www.odva.org/wp-content/uploads/2020/06/PUB00123R1_Common-Industrial_Protocol_and_Family_of_CIP_Networks.pdf](https://www.odva.org/wp-content/uploads/2020/06/PUB00123R1_Common-Industrial_Protocol_and_Family_of_CIP_Networks.pdf)
- **Synopsis**: Essential reading for understanding CIP fundamentals. Explains object-oriented architecture, common services, messaging protocol, communication objects, device profiles, and Electronic Data Sheets (EDS). Describes how CIP adapts to different physical networks (EtherNet/IP, DeviceNet, ControlNet, CompoNet). Covers bridging, routing, and data management. Provides context for how EtherNet/IP implements CIP over Ethernet. Critical for developers implementing CIP objects in EtherNet/IP devices.

## White Papers and Technical Papers

**Rockwell Automation - EtherNet/IP: Industrial Protocol White Paper**
- **Purpose**: Technical overview of EtherNet/IP protocol and implementation
- **Content**: Protocol architecture, messaging types, integration with standard Ethernet, benefits and applications
- **Relevance**: Provides vendor perspective and practical implementation insights
- **Use Case**: Technical overview, protocol understanding, implementation guidance
- **Direct Link**: [https://literature.rockwellautomation.com/idc/groups/literature/documents/wp/enet-wp001_-en-p.pdf](https://literature.rockwellautomation.com/idc/groups/literature/documents/wp/enet-wp001_-en-p.pdf)
- **Synopsis**: Comprehensive white paper covering EtherNet/IP architecture, explicit and implicit messaging, CIP object model, and integration with standard Ethernet infrastructure. Discusses benefits: open standard, standard Ethernet hardware, scalability, real-time performance. Covers application areas: discrete control, process control, motion control, safety. Provides implementation considerations and network design guidelines. Useful for understanding practical aspects of EtherNet/IP deployment.

## Open Source Implementations and Tools

**OpENer - Open Source EtherNet/IP Stack**
- **Purpose**: Open-source EtherNet/IP stack implementation for embedded systems
- **Content**: Source code, documentation, implementation examples, porting guides
- **Relevance**: Reference implementation for EtherNet/IP; our project uses OpENer
- **Use Case**: Reference implementation, code examples, protocol understanding, porting guidance
- **Direct Link**: [https://github.com/EIPStackGroup/OpENer](https://github.com/EIPStackGroup/OpENer)
- **Synopsis**: Portable, open-source EtherNet/IP stack written in C. Supports explicit messaging, implicit I/O, connection management, and CIP objects (Identity, Message Router, Connection Manager, Assembly, TCP/IP Interface, Ethernet Link). Designed for embedded systems with minimal dependencies. Includes documentation, examples, and porting layer for different platforms. Our ESP32-P4 project uses OpENer as the EtherNet/IP stack foundation. Excellent reference for understanding protocol implementation details.

**EIPScanner - EtherNet/IP Scanner Library**
- **Purpose**: Open-source C++ library for EtherNet/IP scanner functionality
- **Content**: Scanner implementation, device discovery, connection management, I/O data exchange
- **Relevance**: Reference for scanner-side implementation and testing tools
- **Use Case**: Testing device implementations, understanding scanner behavior, development tools
- **Direct Link**: [https://github.com/EIPStackGroup/EIPScanner](https://github.com/EIPStackGroup/EIPScanner)
- **Synopsis**: C++ library implementing EtherNet/IP scanner functionality. Supports device discovery, connection establishment, explicit messaging, and implicit I/O. Useful for testing EtherNet/IP devices and understanding scanner-side protocol behavior. Can be used to develop test tools and diagnostic applications. Helps verify device compliance and interoperability.

**EtherNet/IP Protocol Reference (Extracted from Hilscher Scanner Documentation)**
- **Purpose**: Comprehensive protocol reference extracted from Hilscher EtherNet/IP Scanner Protocol API documentation
- **Content**: CIP object details, connection management, messaging types, error codes, protocol fundamentals
- **Relevance**: Useful protocol-level reference for adapter implementation
- **Use Case**: Protocol reference, CIP object details, error code lookup, connection management understanding
- **Direct Link**: [docs/EtherNetIP_Protocol_Reference.md](../docs/EtherNetIP_Protocol_Reference.md)
- **Synopsis**: Extracted protocol information from Hilscher EtherNet/IP Scanner Protocol API V2.10.0. Contains detailed CIP object attributes, services, connection management details, error codes, and protocol fundamentals. While the source document targets scanner implementations, the protocol details are applicable to adapter implementations. Useful as a quick reference for CIP object structures, error codes, and protocol behavior.

**EtherNet/IP Adapter Protocol Reference (Extracted from Hilscher Adapter Documentation V2.13.0)**
- **Purpose**: Adapter-specific protocol reference extracted from Hilscher EtherNet/IP Adapter Protocol API documentation
- **Content**: Adapter responsibilities, connection handling, I/O data processing, status management, error handling
- **Relevance**: **HIGHLY RELEVANT** - Directly applicable to our adapter implementation
- **Use Case**: Adapter implementation guidance, connection management, I/O handling, error codes, best practices
- **Direct Link**: [docs/EtherNetIP_Adapter_Protocol_Reference.md](../docs/EtherNetIP_Adapter_Protocol_Reference.md)
- **Source Document**: [Hilscher EtherNet/IP Adapter Protocol API V2.13.0](https://www.hilscher.com/fileadmin/cms_upload/de/Resources/pdf/EtherNetIP_Adapter_Protocol_API_20_EN.pdf)
- **Synopsis**: Extracted adapter-specific information from Hilscher EtherNet/IP Adapter Protocol API V2.13.0. Contains detailed guidance on adapter responsibilities, Forward_Open/Forward_Close processing, I/O data handling, connection state management, error handling, and implementation best practices. This document is specifically for adapter implementations and provides practical guidance for our ESP32-P4 OpENer adapter project. Highly recommended as the primary reference for adapter-specific protocol behavior.

**EtherNet/IP Adapter V3.4.0 Enhancements**
- **Purpose**: Highlights new features and enhancements in V3.4.0 compared to V2.13.0
- **Content**: Handshake modes, Predefined Connection Object, IO Mapping Object, enhanced connection management, additional services
- **Relevance**: **HIGHLY RELEVANT** - New features that may be useful for advanced I/O synchronization and connection management
- **Use Case**: Advanced features evaluation, handshake mode implementation, I/O mapping, enhanced connection management
- **Direct Link**: [docs/EtherNetIP_Adapter_V3_Enhancements.md](../docs/EtherNetIP_Adapter_V3_Enhancements.md)
- **Source Document**: [Hilscher EtherNet/IP Adapter Protocol API V3.4.0](https://www.hilscher.com/fileadmin/cms_upload/de/Resources/pdf/EtherNetIP_Adapter_V3_Protocol_API_04_EN.pdf)
- **Synopsis**: Extracted new features from Hilscher EtherNet/IP Adapter Protocol API V3.4.0. Key additions include: Handshake Modes (Input/Output/Synchronization) for deterministic I/O synchronization, Predefined Connection Object for faster connection establishment, IO Mapping Object for flexible I/O mapping, and enhanced connection management with Forward_Open/Forward_Close indications. Particularly relevant for Phase 2 (I/O Abstraction Layer) of the ESP32-P4 project. Handshake modes are especially valuable for synchronized, deterministic I/O operations.

**cpppo - EtherNet/IP CIP Python Library**
- **Purpose**: Python library for EtherNet/IP and CIP message construction/parsing
- **Content**: Python API for EtherNet/IP communication, CIP message handling, device interaction
- **Relevance**: Useful for test scripts, diagnostic tools, and automation
- **Use Case**: Test automation, diagnostic tools, protocol analysis, development utilities
- **Direct Link**: [https://github.com/pjkundert/cpppo](https://github.com/pjkundert/cpppo)
- **Synopsis**: Python library for constructing and parsing EtherNet/IP and CIP messages. Provides high-level API for device communication, attribute access, and connection management. Useful for writing test scripts, diagnostic tools, and automation utilities. Can interact with EtherNet/IP devices programmatically. Helpful for development and testing workflows.

## Vendor Documentation and Application Notes

**Rockwell Automation - EtherNet/IP Technical Documentation**
- **Purpose**: Comprehensive technical documentation for EtherNet/IP devices and networks
- **Content**: User manuals, installation guides, application techniques, configuration guides, troubleshooting
- **Relevance**: Practical implementation guidance from major EtherNet/IP vendor
- **Use Case**: Installation procedures, configuration examples, troubleshooting, best practices
- **Direct Link**: [https://www.rockwellautomation.com/en-us/support/documentation/technical/capabilities/ethernet-networks.html](https://www.rockwellautomation.com/en-us/support/documentation/technical/capabilities/ethernet-networks.html)
- **Synopsis**: Extensive collection of technical documentation covering EtherNet/IP network devices, configuration, installation, and troubleshooting. Includes application techniques, user manuals, and reference guides. Provides practical examples and real-world implementation scenarios. Useful for understanding how EtherNet/IP is deployed in industrial environments and common configuration patterns.

**Real Time Automation - EtherNet/IP Protocol Overview**
- **Purpose**: Technical overview and educational resource on EtherNet/IP
- **Content**: Protocol explanation, messaging types, implementation considerations, technical details
- **Relevance**: Educational resource with practical implementation insights
- **Use Case**: Protocol education, technical reference, implementation guidance
- **Direct Link**: [https://www.rtautomation.com/technologies/ethernetip/](https://www.rtautomation.com/technologies/ethernetip/)
- **Synopsis**: Educational resource explaining EtherNet/IP protocol structure, CIP integration, explicit and implicit messaging, and implementation considerations. Provides clear explanations of protocol concepts and practical deployment guidance. Useful for understanding protocol fundamentals and real-world application.

**AutomationDirect - EtherNet/IP Automation Notes**
- **Purpose**: Application notes on EtherNet/IP implementation and evolution
- **Content**: Protocol evolution, implementation considerations, industrial networking trends
- **Relevance**: Industry perspective on EtherNet/IP adoption and implementation
- **Use Case**: Industry context, implementation trends, application considerations
- **Direct Link**: [https://library.automationdirect.com/ethernetip-automation-notes/](https://library.automationdirect.com/ethernetip-automation-notes/)
- **Synopsis**: Application notes discussing the evolution of Ethernet in industrial networks and EtherNet/IP's role. Covers implementation considerations, benefits, and industry trends. Provides context for EtherNet/IP adoption and practical deployment guidance.

## Standards and RFCs

**RFC 5227 - IPv4 Address Conflict Detection**
- **Purpose**: Standard for detecting IPv4 address conflicts before assignment
- **Content**: Address Conflict Detection (ACD) protocol specification, probe mechanism, conflict resolution
- **Relevance**: **CRITICAL** - Directly relevant to our static IP ACD implementation
- **Use Case**: ACD implementation reference, conflict detection algorithm, compliance verification
- **Direct Link**: [https://www.rfc-editor.org/rfc/rfc5227](https://www.rfc-editor.org/rfc/rfc5227)
- **Synopsis**: **ESSENTIAL FOR OUR PROJECT**: Defines Address Conflict Detection (ACD) protocol for IPv4. Specifies probe mechanism using ARP to detect address conflicts before assignment. Defines timing requirements: PROBE_WAIT, PROBE_MIN, PROBE_MAX, PROBE_NUM, ANNOUNCE_NUM. Our LWIP implementation follows RFC 5227 for static IP assignment. Device must probe network before assigning static IP; if conflict detected, must NOT use address. This RFC validates our approach and provides implementation requirements.

**IEEE 802.3 - Ethernet Standard**
- **Purpose**: Standard for Ethernet physical and data link layer specifications
- **Content**: Physical layer specifications, MAC layer, frame format, media access control
- **Relevance**: Foundation for EtherNet/IP physical layer
- **Use Case**: Physical layer implementation, Ethernet frame format, media specifications
- **Direct Link**: [https://standards.ieee.org/standard/802_3-2018.html](https://standards.ieee.org/standard/802_3-2018.html)
- **Synopsis**: IEEE standard defining Ethernet physical and data link layers. EtherNet/IP uses standard Ethernet frames (IEEE 802.3) for transport. Defines frame structure, MAC addresses, CSMA/CD (for half-duplex), and physical media specifications. Understanding Ethernet fundamentals is essential for EtherNet/IP network design and troubleshooting.

**RFC 791 - Internet Protocol (IPv4)**
- **Purpose**: IPv4 protocol specification
- **Content**: IP packet format, addressing, routing, fragmentation
- **Relevance**: Foundation for EtherNet/IP IP layer
- **Use Case**: IP layer implementation, addressing, packet format
- **Direct Link**: [https://www.rfc-editor.org/rfc/rfc791](https://www.rfc-editor.org/rfc/rfc791)
- **Synopsis**: Defines IPv4 protocol used by EtherNet/IP. Specifies IP packet format, addressing scheme, and routing principles. EtherNet/IP uses standard IP addressing and routing. Essential for understanding IP layer behavior in EtherNet/IP networks.

**RFC 768 - User Datagram Protocol (UDP)**
- **Purpose**: UDP protocol specification
- **Content**: UDP packet format, port numbers, checksum
- **Relevance**: Used by EtherNet/IP for implicit (I/O) messaging
- **Use Case**: UDP implementation, implicit messaging transport
- **Direct Link**: [https://www.rfc-editor.org/rfc/rfc768](https://www.rfc-editor.org/rfc/rfc768)
- **Synopsis**: Defines UDP protocol used by EtherNet/IP for implicit (real-time I/O) messaging. UDP provides connectionless, low-overhead transport for cyclic I/O data. EtherNet/IP uses UDP port 2222 for implicit messaging. Understanding UDP is essential for implementing implicit I/O connections.

**RFC 793 - Transmission Control Protocol (TCP)**
- **Purpose**: TCP protocol specification
- **Content**: TCP connection establishment, reliable delivery, flow control, port numbers
- **Relevance**: Used by EtherNet/IP for explicit messaging
- **Use Case**: TCP implementation, explicit messaging transport
- **Direct Link**: [https://www.rfc-editor.org/rfc/rfc793](https://www.rfc-editor.org/rfc/rfc793)
- **Synopsis**: Defines TCP protocol used by EtherNet/IP for explicit (request-response) messaging. TCP provides reliable, connection-oriented transport for configuration and diagnostics. EtherNet/IP uses TCP port 44818 for explicit messaging. Understanding TCP is essential for implementing explicit messaging and connection management.

## Educational and Reference Resources

**ODVA Technology Standards - EtherNet/IP Overview**
- **Purpose**: Official ODVA overview of EtherNet/IP technology
- **Content**: Technology introduction, features, benefits, applications, technical specifications
- **Relevance**: Official ODVA resource for understanding EtherNet/IP
- **Use Case**: Technology overview, feature reference, official information
- **Direct Link**: [https://www.odva.org/technology-standards/key-technologies/ethernet-ip/](https://www.odva.org/technology-standards/key-technologies/ethernet-ip/)
- **Synopsis**: Official ODVA page providing comprehensive overview of EtherNet/IP technology. Covers features, benefits, applications, and technical specifications. Links to related resources, specifications, and documentation. Essential starting point for understanding EtherNet/IP from the standards organization perspective.

**Wikipedia - EtherNet/IP Article**
- **Purpose**: General reference article on EtherNet/IP
- **Content**: Protocol overview, history, technical details, related technologies, applications
- **Relevance**: General reference and quick overview
- **Use Case**: Quick reference, general understanding, historical context
- **Direct Link**: [https://en.wikipedia.org/wiki/EtherNet/IP](https://en.wikipedia.org/wiki/EtherNet/IP)
- **Synopsis**: Wikipedia article providing general overview of EtherNet/IP protocol. Covers history, technical details, messaging types, and applications. Includes links to related protocols and technologies. Useful for quick reference and general understanding, though not a substitute for official specifications.

**Delta Motion - EtherNet/IP Overview**
- **Purpose**: Motion control perspective on EtherNet/IP
- **Content**: EtherNet/IP in motion control applications, messaging, I/O capabilities
- **Relevance**: Application-specific perspective on EtherNet/IP
- **Use Case**: Motion control applications, application examples, implementation insights
- **Direct Link**: [https://deltamotion.com/support/webhelp/rmctools/Content/Communications/Ethernet/Supported_Protocols/EtherNetIP/EtherNet_IP.htm](https://deltamotion.com/support/webhelp/rmctools/Content/Communications/Ethernet/Supported_Protocols/EtherNetIP/EtherNet_IP.htm)
- **Synopsis**: Overview of EtherNet/IP from motion control perspective. Explains how EtherNet/IP is used in motion control applications, messaging capabilities, and I/O features. Provides application-specific insights and implementation considerations. Useful for understanding EtherNet/IP in motion control contexts.

## Accessing These Publications

These publications are available through ODVA's Document Library:
- **ODVA Website**: https://www.odva.org/technology-standards/document-library/
- **Access**: Some publications may require ODVA membership or purchase
- **Public Resources**: Some overview documents and guides are publicly available

## Related Specifications

For complete technical implementation details, refer to:
- **EtherNet/IP Specification** (Volume 1: CIP, Volume 2: EtherNet/IP Adaptation) - Available through ODVA membership
- **CIP Specification Volume 1** - Common Industrial Protocol (CIP) - Available through ODVA membership
- **PUB00123R1** - "The Common Industrial Protocol (CIP™) and the Family of CIP Networks" - See "Additional ODVA Core Publications" section above

## Project-Specific Relevance

### High Priority (Essential)
- **Pub 138** - Technology Overview (foundational understanding)
- **Pub 95** - Interoperability Test Procedures (conformance testing)
- **Pub 28** - IP Addressing Methods (directly relevant to our ACD implementation)
- **Pub 70** - Recommended Functionality (feature compliance)

### Medium Priority (Important)
- **Pub 80/81** - Performance Testing (performance validation)
- **Pub 148** - Media Planning (network infrastructure)
- **Pub 35** - Network Infrastructure Guide (network design)
- **Pub 213** - Quick Start (developer onboarding)

### Low Priority (Reference)
- **Pub 88** - DHCP Relay Agent (if DHCP support is added)
- **Pub 316** - DLR Guidelines (future feature consideration)

### Additional High-Value Resources
- **PUB00123R1** - CIP Overview (foundational CIP understanding)
- **RFC 5227** - ACD Specification (critical for our implementation)
- **OpENer GitHub** - Reference implementation (our stack foundation)
- **Rockwell White Paper** - Practical implementation insights
- **RFC 768/793** - UDP/TCP specifications (transport layer understanding)

## Notes

- These publications are maintained by ODVA and updated periodically
- Always refer to the latest versions for current specifications
- Some publications may have version-specific requirements
- ODVA membership provides access to the complete specification library

## Additional Resources

- **ODVA Website**: https://www.odva.org/
- **ODVA Technology Standards**: https://www.odva.org/technology-standards/
- **ODVA Learning Tools**: https://www.odva.org/subscriptions-services/learning-tools/
- **PlugFest Events**: ODVA-sponsored interoperability testing events

---

*Last Updated: Based on ODVA publications as of project documentation date*
*Project: ESP32-P4 EtherNet/IP Implementation*

