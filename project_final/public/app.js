
const editor = document.getElementById('editor');
const preview = document.getElementById('preview');
const convertBtn = document.getElementById('convertBtn');
const downloadBtn = document.getElementById('downloadBtn');
const status = document.getElementById('status');

let convertedHTML = '';

convertBtn.addEventListener('click', async function () {
    const markdown = editor.value;

    if (markdown.trim() === '') {
        status.textContent = 'Please write some Markdown first.';
        status.className = 'error';
        return;
    }

    status.textContent = 'Converting Markdown...';
    status.className = '';
    convertBtn.disabled = true;

    const response = await fetch('/convert', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ markdown: markdown })
    });

    const data = await response.json();

    if (response.ok) {
        convertedHTML = data.html;
        preview.innerHTML = convertedHTML;

        downloadBtn.disabled = false;
        status.textContent = 'Markdown converted successfully!';
        status.className = 'success';
    } else {
        status.textContent = data.error || 'Conversion failed.';
        status.className = 'error';
    }

    convertBtn.disabled = false;
});

downloadBtn.addEventListener('click', function () {
    if (convertedHTML === '') {
        return;
    }

    const fullHTML = `<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Converted Markdown</title>
</head>
<body>
${convertedHTML}
</body>
</html>`;

    const file = new Blob([fullHTML], {
        type: 'text/html;charset=utf-8'
    });

    const url = URL.createObjectURL(file);
    const link = document.createElement('a');

    link.href = url;
    link.download = 'converted.html';
    link.click();

    URL.revokeObjectURL(url);
});
