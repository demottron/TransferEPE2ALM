# TransferEPE2ALM

A Windows desktop application that transfers test run data from Enterprise Performance Engineering (EPE, formerly LoadRunner Enterprise/LRE) to Application Lifecycle Management (ALM).

## Description

TransferEPE2ALM pulls test run data out of EPE using REST API calls and inserts it into ALM's corresponding fields, creating a Test Instance record set. The application handles authentication with both systems, retrieves scenario execution data, creates test instances, and updates run statuses.

The application serves as a bridge between the performance testing system (EPE) and the test management system (ALM), automating the process of transferring test results and maintaining consistency between both platforms.

## Features

- Authenticates with both EPE and ALM systems using secure API calls
- Transfers complete test run data between systems
- Supports multiple test run transfers (single, range, or comma-separated IDs)
- Updates test instance and run statuses in ALM
- Configurable via command line parameters
- Registry-based server configuration

## Getting Started

### Prerequisites

- Windows operating system
- Visual Studio 2022 (or compatible version)
- Access credentials for EPE and ALM systems
- Proper registry configuration for server endpoints

### Installation

1. Clone the repository:
   ```
   git clone https://github.com/yourusername/TransferEPE2ALM.git
   ```

2. Open the solution in Visual Studio.

3. Build the solution.

4. Make sure server names are properly configured in the registry:
   - Key: `HKEY_CURRENT_USER\SOFTWARE\TransferLREtoALM`
   - Values:
     - `ALMserver` - URL for the ALM server
     - `EPEserver` - URL for the EPE server

### Usage

The application requires several command line parameters:

```
TransferEPE2ALM -Project <project_name> -Run <run_id> -Owner <owner_id> -TestType <test_type> -Status <status> -TestID <test_id> -TestSetID <testset_id>
```

Parameters:
- `Project`: Name of the EPE project to target (must match the parent folder name in ALM)
- `Run`: Test execution ID(s) to target (single ID, range like xxxx-yyyy, or comma-separated values)
- `Owner`: Network ID of the test owner
- `TestType`: Type of test (Baseline, Endurance, Increasing Load, Smoke)
- `Status`: Test status (Completed, Completed With Findings, Blocked, Failed, N/A, NoRun, Not Completed, Passed)
- `TestID`: ID of the TEST entity in ALM's Test Plan
- `TestSetID`: ID of the TESTSET entity in ALM's Test Lab

## Architecture

The application follows a modular architecture with the following key components:

- `initialize_Program.c`: Handles command-line arguments and initial setup
- `ALM_init.c`: Handles ALM authentication and parameter setup
- `Get_LRE_Info.c`/`Get_LRE_Extended.c`: Retrieves test data from EPE
- `Get_ALM_Info.c`: Retrieves required field information from ALM
- `POST_test_instance_data.c`: Creates test instance records in ALM
- `PUT_run_data.c`: Updates test run statuses in ALM
- `pttw_HTTP_functions.c`: HTTP/REST utility functions
- `pttw_util_functions.c`: General utility functions

## API Documentation

The application uses the following REST APIs:

- [EPE REST API Documentation](https://admhelp.microfocus.com/lre/en/all/api_refs/Performance_Center_REST_API/Content/Welcome.htm)
- [ALM REST API Documentation](https://admhelp.microfocus.com/alm/api_refs/REST_core/Content/REST_API_Core/General/Overview.html)

## Contributing

Contributions to this project are welcome. Please ensure your code follows the existing patterns and includes appropriate documentation.

## Version History

* 1.1.9
  * Current version with fixed compiler warnings
* 1.1.8
  * Previous stable version

## TODO List

- [ ] Update error handling for HTTP failures
- [ ] Implement a graphical user interface for parameter input
- [ ] Add logging capabilities
- [ ] Implement certificate validation for HTTPS connections
- [ ] Add unit tests
- [ ] Create installation package
- [ ] Add support for newer EPE/ALM API versions
- [ ] Improve documentation
- [ ] Support additional authentication methods

## License

This project is licensed under the appropriate license for your organization - consult your legal department for details.

## Acknowledgments

* VA Performance Testing Team
* Micro Focus (now OpenText) for EPE and ALM APIs