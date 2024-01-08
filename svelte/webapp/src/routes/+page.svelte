<script>
    import { page } from "$app/stores";
    import PapaParse from "papaparse";
    import regression from "regression";

    let pageObj = [
        {
            id: 0,
            channel: "CH0",
            unit: "",
            pos: "",
            name: "",
            max: 0,
            min: 0,
            coeff: [0, 0, 0, 0],
        },
        {
            id: 1,
            channel: "CH1",
            unit: "",
            pos: "",
            name: "",
            max: 0,
            min: 0,
            coeff: [0, 0, 0, 0],
        },
        {
            id: 2,
            channel: "CH2",
            unit: "",
            pos: "",
            name: "",
            max: 0,
            min: 0,
            coeff: [0, 0, 0, 0],
        },
        {
            id: 3,
            channel: "CH3",
            unit: "",
            pos: "",
            name: "",
            max: 0,
            min: 0,
            coeff: [0, 0, 0, 0],
        },
        {
            id: 4,
            channel: "CH4",
            unit: "",
            pos: "",
            name: "",
            max: 0,
            min: 0,
            coeff: [0, 0, 0, 0],
        },
        {
            id: 5,
            channel: "CH5",
            unit: "",
            pos: "",
            name: "",
            max: 0,
            min: 0,
            coeff: [0, 0, 0, 0],
        },
        {
            id: 6,
            channel: "CH6",
            unit: "",
            pos: "",
            name: "",
            max: 0,
            min: 0,
            coeff: [0, 0, 0, 0],
        },
        {
            id: 7,
            channel: "CH7",
            unit: "",
            pos: "",
            name: "",
            max: 0,
            min: 0,
            coeff: [0, 0, 0, 0],
        },
    ];

    let channelSelected = "CH0";
    let unitSelected = "Psi";
    let displaySelected = "A";
    let max = 0;
    let min = 0;
    let name = "";

    let dataToCSV = [];
    let prevDataToCSV;

    export let allowedFileExtensions = ["csv"];

    let maxFileSize = 3145728;

    // this is the variable that the file gets bound to
    let uploader;

    $: {
        // called on props change
        if (dataToCSV !== prevDataToCSV && dataToCSV.length !== 0) {
            prevDataToCSV = dataToCSV;
            const csvData = PapaParse.unparse(dataToCSV);
            const result = regression.polynomial(csvData, { order: 3 });
            console.log("result");
            // console.log(result);
            onUpload
                ? onUpload(csvData)
                : console.log(
                      "Remember to define an onUpload function as props. CSV Data:",
                      csvData
                  );
        }
    }

    function onUpload(file) {
        //console.log("this is csv file", file, file.length - 2);
        const data = [];

        for (let index = 0; index < file.length - 1; index++) {
            data.push([parseFloat(file[index][0]), parseFloat(file[index][1])]);
        }
        //console.log(data);
        const result = regression.polynomial(data, { order: 3 });

        var filterOBJ = pageObj.find(function (item, i) {
            if (item.channel == channelSelected) return i;
        });

        if (channelSelected == "CH0") filterOBJ = pageObj[0];

        filterOBJ.coeff[0] = result.equation[0];
        filterOBJ.coeff[1] = result.equation[1];
        filterOBJ.coeff[2] = result.equation[2];
        filterOBJ.coeff[3] = result.equation[3];

        console.log(channelSelected);
        console.log(filterOBJ);
        console.log(result.equation);
    }

    function uploadFile(event) {
        event.preventDefault();
        const file = uploader.files[0];

        const fileExtensionArray = file.type.split("/");
        const fileExtension = fileExtensionArray[fileExtensionArray.length - 1];

        if (file.size > maxFileSize) {
            console.log("Above the max file size threshold");
            return;
        }
        if (fileExtension.includes("csv") && file.size < maxFileSize) {
            const csvData = PapaParse.parse(file, {
                complete: (results) => {
                    onUpload
                        ? onUpload(results.data)
                        : console.log(
                              "Remember to define an onUpload function as props. Parsed CSV:",
                              results.data
                          );
                },
            });
        } else if (allowedFileExtensions.includes(fileExtension)) {
            onUpload
                ? onUpload(file)
                : console.log(
                      "Remember to define an onUpload function as props. Plain file:",
                      file
                  );
        } else {
            console.log("Not an allowed file type");
        }
    }

    async function saveInfo() {
        var filterOBJ = pageObj.find(function (item, i) {
            if (item.channel == channelSelected) return i;
        });
        if (channelSelected == "CH0") filterOBJ = pageObj[0];
        console.log("value of filter obj", filterOBJ);
        filterOBJ.max = max;
        filterOBJ.min = min;
        filterOBJ.name = name;
        filterOBJ.pos = displaySelected;
        filterOBJ.unit = unitSelected;
        const res = await fetch("/save", {
            method: "POST",
            body: JSON.stringify(filterOBJ),
        });
        const json = await res.json();
        console.log(res);
        console.log(json);
    }

    async function preview() {
        const res = await fetch("/preview", {
            method: "POST",
            body: channelSelected,
        });
        const filterOBJ = await res.json();
        console.log(filterOBJ);
        max = filterOBJ.max;
        min = filterOBJ.min;
        name = filterOBJ.name;
        displaySelected = filterOBJ.pos;
        unitSelected = filterOBJ.unit;
    }
</script>

<div class="grid grid-cols-2 content-center mt-20 ml-20">
    <div class="mb-10 mt-5">
        <select
            class="select select-bordered w-full max-w-xs"
            bind:value={channelSelected}
        >
            <option value="CH0">CH0</option>
            <option value="CH1">CH1</option>
            <option value="CH2">CH2</option>
            <option value="CH3">CH3</option>
            <option value="CH4">CH4</option>
            <option value="CH5">CH5</option>
            <option value="CH6">CH6</option>
            <option value="CH7">CH7</option>
        </select>
    </div>

    <div>
        <label class="ml-5" for="idname">Name</label>
        <input
            id="idname"
            type="text"
            placeholder="Type here"
            class="flex input input-bordered w-full max-w-xs"
            bind:value={name}
        />
    </div>

    <div>
        <label class="ml-5">Upload csv</label>
        <input
            class="file-input file-input-bordered w-full max-w-xs flex"
            type="file"
            accept=".csv"
            bind:this={uploader}
            on:change={uploadFile}
        />
    </div>

    <div>
        <label class="ml-5" for="idunit">Unit</label>
        <select
            id="idunit"
            class="select select-bordered w-full max-w-xs flex"
            bind:value={unitSelected}
        >
            <option value="Psi">Psi</option>
            <option value="BAR">BAR</option>
            <option value="Celsius">Celsius</option>
            <option value="Fahrenheit">Fahrenheit</option>
            <option value="Lambda">Lambda</option>
            <option value="AFR">AFR</option>
            <option value="Voltage">Voltage</option>
        </select>
    </div>

    <div class="mt-10">
        <label class="ml-5">Display Option</label>
        <select
            class="select select-bordered w-full max-w-xs flex"
            bind:value={displaySelected}
        >
            <option value="A">A</option>
            <option value="B">B</option>
            <option value="C">C</option>
            <option value="D">D</option>
            <option value="E">E</option>
            <option value="F">F</option>
            <option value="G">G</option>
            <option value="H">H</option>
        </select>
    </div>

    <div class="mt-10">
        <label class="ml-5">MAX</label>
        <input
            type="number"
            placeholder="MAX"
            class="input input-bordered w-50 max-w-xs flex"
            bind:value={max}
        />
        <label class="ml-5">MIN</label>
        <input
            type="number"
            placeholder="MIN"
            class="input input-bordered w-50 max-w-xs flex"
            bind:value={min}
        />
    </div>

    <div class="content-center ml-10">
        <button class="btn" on:click={preview}>Preview</button>

        <button class="btn" on:click={saveInfo}>Save</button>
    </div>
</div>
