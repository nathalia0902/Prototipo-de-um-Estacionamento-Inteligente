const source = new EventSource('/updates');
source.onmessage = e => {
const data = JSON.parse(e.data);
document.getElementById('vaga1').classList.toggle('ocupada', data.vaga1);
document.getElementById('vaga2').classList.toggle('ocupada', data.vaga2);
};